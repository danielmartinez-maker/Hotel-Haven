import csv
import subprocess
import sys


def main() -> int:
    if len(sys.argv) != 2:
        raise SystemExit("usage: headless_campaign_restock_test.py <headless-executable>")

    result = subprocess.run(
        [sys.argv[1], "--days", "60", "--restock"],
        check=False,
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        raise AssertionError(f"campaign exited {result.returncode}: {result.stderr}")

    rows = list(csv.DictReader(result.stdout.splitlines()))
    if len(rows) != 60:
        raise AssertionError(f"expected 60 daily campaign rows, got {len(rows)}")

    completed_stays = int(rows[-1]["completed_stays"])
    final_cash = int(rows[-1]["cash_cents"])
    blocked_days = [
        int(row["day"]) for row in rows if int(row["blocked_tasks"]) > 0
    ]
    if completed_stays < 60 or final_cash <= 0 or blocked_days:
        raise AssertionError(
            "automatic restocking stalled the starter hotel: "
            f"completed_stays={completed_stays}, cash_cents={final_cash}, "
            f"blocked_days={blocked_days}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

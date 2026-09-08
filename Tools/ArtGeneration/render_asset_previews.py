from __future__ import annotations
import argparse,json,math
from pathlib import Path
import numpy as np
import trimesh
from PIL import Image,ImageDraw

BG=(244,242,237);BORDER=(205,201,194);TEXT=(45,45,45)

def rot_z(a):
 c,s=math.cos(a),math.sin(a);return np.array([[c,-s,0.0],[s,c,0.0],[0.0,0.0,1.0]])
def rot_x(a):
 c,s=math.cos(a),math.sin(a);return np.array([[1.0,0.0,0.0],[0.0,c,-s],[0.0,s,c]])
VIEW=rot_x(math.radians(35.264))@rot_z(math.radians(45.0))
LIGHT=np.array([0.35,-0.45,0.82]);LIGHT=LIGHT/np.linalg.norm(LIGHT)

def material_rgb(geom):
 mat=getattr(getattr(geom,'visual',None),'material',None);value=getattr(mat,'baseColorFactor',None)
 if value is None:return np.array([170.0,160.0,145.0])
 arr=np.asarray(value,dtype=float).reshape(-1)
 if arr.size<3:return np.array([170.0,160.0,145.0])
 if arr.max()<=1.01:arr=arr*255.0
 return np.clip(arr[:3],0,255)

def render_asset(path:Path,size:int=150):
 scene=trimesh.load(path,force='scene');triangles=[];points=[]
 for node in scene.graph.nodes_geometry:
  transform,gname=scene.graph.get(node);geom=scene.geometry[gname]
  verts=trimesh.transform_points(geom.vertices,transform);verts=(VIEW@verts.T).T;points.append(verts)
  face_verts=verts[geom.faces];normal=np.cross(face_verts[:,1]-face_verts[:,0],face_verts[:,2]-face_verts[:,0]);length=np.linalg.norm(normal,axis=1,keepdims=True);length[length==0]=1;normal=normal/length
  shade=.58+.42*np.clip(normal@LIGHT,0,1);base=material_rgb(geom)
  for tri,sh in zip(face_verts,shade):triangles.append((float(tri[:,1].mean()),tri,np.clip(base*sh,25,245)))
 all_points=np.vstack(points);projected=np.column_stack([all_points[:,0],all_points[:,2]]);mn,mx=projected.min(axis=0),projected.max(axis=0);center=(mn+mx)/2;scale=(size*.72)/max(float((mx-mn).max()),1e-6)
 def project(tri):
  q=np.column_stack([tri[:,0],tri[:,2]]);q=(q-center)*scale;q[:,0]+=size/2;q[:,1]=size/2-q[:,1];return [tuple(x) for x in q]
 image=Image.new('RGB',(size,size),BG);draw=ImageDraw.Draw(image)
 for _,tri,color in sorted(triangles,key=lambda item:item[0]):draw.polygon(project(tri),fill=tuple(color.astype(int)))
 return image

def names_for_batch(manifest:Path):
 data=json.loads(manifest.read_text());return {row[0]:row[1] for group in data['groups'] for row in group['assets']}

def contact_sheet(batch_dir:Path,manifest:Path,out:Path,cell:int=160,cols:int=10):
 paths=sorted(batch_dir.glob('HH_A*.glb'));names=names_for_batch(manifest);rows=math.ceil(len(paths)/cols);label_h=36
 sheet=Image.new('RGB',(cols*cell,rows*(cell+label_h)),BG);draw=ImageDraw.Draw(sheet)
 for i,path in enumerate(paths):
  x=(i%cols)*cell;y=(i//cols)*(cell+label_h);sheet.paste(render_asset(path,cell),(x,y));draw.rectangle((x,y,x+cell-1,y+cell+label_h-1),outline=BORDER,width=1)
  draw.text((x+5,y+cell+2),path.stem,fill=TEXT);label=names.get(path.stem,'')[:24];draw.text((x+5,y+cell+17),label,fill=TEXT)
 out.parent.mkdir(parents=True,exist_ok=True);sheet.save(out,optimize=True)

def generate(repo_root:Path):
 for i in range(1,11):
  contact_sheet(repo_root/'Art/Exports'/f'Batch{i:02d}',repo_root/'GameData/AssetDefinitions/Manifest'/f'asset_batch_{i:02d}.json',repo_root/'Art/Validation/Previews'/f'Batch{i:02d}.png')
 print('generated 10 batch preview sheets')

def main():
 p=argparse.ArgumentParser();p.add_argument('repo_root',nargs='?',default='.');a=p.parse_args();generate(Path(a.repo_root).resolve())
if __name__=='__main__':main()

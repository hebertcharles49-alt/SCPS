# recut.ps1 - re-cut recentered parchment cells (ASCII-only for PS 5.1).
# Cause: the rigid 256 grid is slightly offset -> each cell eats a sliver of the
# neighbor (and clips its own opposite edge). Fix: proven magenta key (port of
# rekey.ps1) on the WHOLE master, then per cell: padded window -> connected
# components -> keep the one(s) centered in the nominal cell (neighbor sliver, in
# the pad band, is dropped) -> trim -> RECENTER on the cell. Tile/frame guard: if
# content hits all 4 edges strongly (full biome / 9-slice) -> rigid slice (no
# recenter). Params: -SheetFilter, -OutDir (empty = overwrite repo in place), -Pad.
param(
  [string]$SheetFilter = "*",
  [string]$OutDir = "",
  [int]$Pad = 44
)
$ErrorActionPreference = "Stop"

Add-Type -ReferencedAssemblies System.Drawing @"
using System; using System.Collections.Generic; using System.Drawing;
using System.Drawing.Imaging; using System.Runtime.InteropServices;
public static class Recut {
  static bool Strong(byte[] p,int i){int b=p[i*4],g=p[i*4+1],r=p[i*4+2];int m=Math.Min(r,b)-g;return r>=170&&b>=170&&g<=110&&m>=90;}
  static bool Ish(byte[] p,int i){int b=p[i*4],g=p[i*4+1],r=p[i*4+2];int m=Math.Min(r,b)-g;return m>=45&&r>=120&&b>=110;}
  static bool Ish2(byte[] p,int i){int b=p[i*4],g=p[i*4+1],r=p[i*4+2];int m=Math.Min(r,b)-g;return m>=45&&r>=155&&b>=150;}
  static void Key(byte[] px,int w,int h){
    int n=w*h; var key=new bool[n];
    for(int i=0;i<n;i++) if(Strong(px,i)) key[i]=true;
    var vis=new bool[n]; var st=new Stack<int>();
    for(int x=0;x<w;x++){int[] s={x,(h-1)*w+x};foreach(int i in s) if(!vis[i]&&Ish(px,i)){vis[i]=true;st.Push(i);}}
    for(int y=0;y<h;y++){int[] s={y*w,y*w+w-1};foreach(int i in s) if(!vis[i]&&Ish(px,i)){vis[i]=true;st.Push(i);}}
    while(st.Count>0){int i=st.Pop();key[i]=true;int x=i%w,y=i/w;
      if(x>0&&!vis[i-1]&&Ish(px,i-1)){vis[i-1]=true;st.Push(i-1);}
      if(x<w-1&&!vis[i+1]&&Ish(px,i+1)){vis[i+1]=true;st.Push(i+1);}
      if(y>0&&!vis[i-w]&&Ish(px,i-w)){vis[i-w]=true;st.Push(i-w);}
      if(y<h-1&&!vis[i+w]&&Ish(px,i+w)){vis[i+w]=true;st.Push(i+w);}}
    var vis2=new bool[n];
    for(int i=0;i<n;i++) if(key[i]){vis2[i]=true;st.Push(i);}
    while(st.Count>0){int i=st.Pop();int x=i%w,y=i/w;
      if(x>0&&!vis2[i-1]&&Ish2(px,i-1)){vis2[i-1]=true;key[i-1]=true;st.Push(i-1);}
      if(x<w-1&&!vis2[i+1]&&Ish2(px,i+1)){vis2[i+1]=true;key[i+1]=true;st.Push(i+1);}
      if(y>0&&!vis2[i-w]&&Ish2(px,i-w)){vis2[i-w]=true;key[i-w]=true;st.Push(i-w);}
      if(y<h-1&&!vis2[i+w]&&Ish2(px,i+w)){vis2[i+w]=true;key[i+w]=true;st.Push(i+w);}}
    var extra=new List<int>();
    for(int y=0;y<h;y++)for(int x=0;x<w;x++){int i=y*w+x;if(key[i])continue;
      int b=px[i*4],g=px[i*4+1],r=px[i*4+2];int m=Math.Min(r,b)-g;if(m<45)continue;
      int tot=0,kd=0;for(int dy=-3;dy<=3;dy++)for(int dx=-3;dx<=3;dx++){int xx=x+dx,yy=y+dy;if(xx<0||xx>=w||yy<0||yy>=h)continue;tot++;if(key[yy*w+xx])kd++;}
      if(kd*100>=tot*40) extra.Add(i);}
    foreach(int i in extra) key[i]=true;
    var fr=new bool[n];
    for(int y=0;y<h;y++)for(int x=0;x<w;x++){int i=y*w+x;if(key[i])continue;
      for(int dy=-3;dy<=3;dy++){for(int dx=-3;dx<=3;dx++){int xx=x+dx,yy=y+dy;if(xx>=0&&xx<w&&yy>=0&&yy<h&&key[yy*w+xx]){fr[i]=true;dy=4;break;}}}}
    for(int i=0;i<n;i++){
      if(key[i]){px[i*4+3]=0;continue;}
      if(!fr[i])continue;
      int b=px[i*4],g=px[i*4+1],r=px[i*4+2];int m=Math.Min(r,b)-g;
      if(m>20){bool bright=r>=160;double f=bright?0.15:0.35;
        px[i*4+2]=(byte)(g+(int)((r-g)*f));px[i*4]=(byte)(g+(int)((b-g)*f));
        int a=255-(m-20)*(bright?3:2);int fl=bright?60:90;if(a<fl)a=fl;if(a<px[i*4+3])px[i*4+3]=(byte)a;}}
  }
  static byte[] _px; static int _w,_h;
  public static void Load(string master, bool chroma){
    using(var orig=new Bitmap(master))
    using(var bmp=new Bitmap(orig.Width,orig.Height,PixelFormat.Format32bppArgb)){
      _w=bmp.Width;_h=bmp.Height;int n=_w*_h;
      using(var gr=Graphics.FromImage(bmp)) gr.DrawImage(orig,0,0,_w,_h);
      var d=bmp.LockBits(new Rectangle(0,0,_w,_h),ImageLockMode.ReadWrite,PixelFormat.Format32bppArgb);
      _px=new byte[n*4];Marshal.Copy(d.Scan0,_px,0,_px.Length);bmp.UnlockBits(d);
    }
    if(chroma) Key(_px,_w,_h);
  }
  public static string Cell(int col,int row,int cw,int ch,int pad,string dst){
    int x0=col*cw, y0=row*ch;
    int lp=Math.Min(pad,x0), tp=Math.Min(pad,y0);
    int rp=Math.Min(pad,_w-(x0+cw)), bp=Math.Min(pad,_h-(y0+ch));
    int wx=x0-lp, wy=y0-tp, ww=cw+lp+rp, wh=ch+tp+bp;
    int wn=ww*wh;
    var a=new byte[wn]; var col4=new byte[wn*4];
    for(int y=0;y<wh;y++)for(int x=0;x<ww;x++){
      int si=((wy+y)*_w+(wx+x))*4, di=(y*ww+x);
      col4[di*4]=_px[si];col4[di*4+1]=_px[si+1];col4[di*4+2]=_px[si+2];col4[di*4+3]=_px[si+3];
      a[di]=_px[si+3];
    }
    int e=0,et=0;
    for(int x=0;x<cw;x++){ et+=2; if(a[(tp)*ww+(lp+x)]>40)e++; if(a[(tp+ch-1)*ww+(lp+x)]>40)e++; }
    for(int y=0;y<ch;y++){ et+=2; if(a[(tp+y)*ww+(lp)]>40)e++; if(a[(tp+y)*ww+(lp+cw-1)]>40)e++; }
    bool tile = et>0 && (double)e/et > 0.72;
    var lab=new int[wn]; for(int i=0;i<wn;i++) lab[i]=-1;
    var cx=new List<double>(); var cy=new List<double>(); var cnt=new List<int>();
    var st=new Stack<int>(); int nl=0;
    for(int i=0;i<wn;i++){
      if(a[i]<=40||lab[i]>=0) continue;
      lab[i]=nl; st.Push(i); double sx=0,sy=0; int c=0;
      while(st.Count>0){int j=st.Pop(); int jx=j%ww, jy=j/ww; sx+=jx; sy+=jy; c++;
        for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++){
          if(dx==0&&dy==0)continue; int xx=jx+dx,yy=jy+dy;
          if(xx<0||xx>=ww||yy<0||yy>=wh)continue; int k=yy*ww+xx;
          if(a[k]>40&&lab[k]<0){lab[k]=nl;st.Push(k);}}}
      cx.Add(sx/c); cy.Add(sy/c); cnt.Add(c); nl++;
    }
    int cellArea=cw*ch; int minArea=(int)(cellArea*0.0006);
    var keep=new bool[Math.Max(nl,1)];
    int kept=0;
    for(int l=0;l<nl;l++){
      if(cnt[l]<minArea) continue;
      double px_=cx[l], py_=cy[l];
      if(px_>=lp && px_<lp+cw && py_>=tp && py_<tp+ch){ keep[l]=true; kept++; }
    }
    var outbmp=new Bitmap(cw,ch,PixelFormat.Format32bppArgb);
    var od=outbmp.LockBits(new Rectangle(0,0,cw,ch),ImageLockMode.WriteOnly,PixelFormat.Format32bppArgb);
    var op=new byte[cw*ch*4];
    if(tile || kept==0){
      for(int y=0;y<ch;y++)for(int x=0;x<cw;x++){
        int si=((tp+y)*ww+(lp+x))*4, di=(y*cw+x)*4;
        op[di]=col4[si];op[di+1]=col4[si+1];op[di+2]=col4[si+2];op[di+3]=col4[si+3];}
    } else {
      int bx0=ww,by0=wh,bx1=-1,by1=-1;
      for(int y=0;y<wh;y++)for(int x=0;x<ww;x++){int i=y*ww+x;int l=lab[i];
        if(l<0||!keep[l])continue; if(x<bx0)bx0=x;if(x>bx1)bx1=x;if(y<by0)by0=y;if(y>by1)by1=y;}
      int bw=bx1-bx0+1, bh=by1-by0+1;
      int dstx=(cw-bw)/2, dsty=(ch-bh)/2;
      for(int y=0;y<bh;y++)for(int x=0;x<bw;x++){
        int sx=bx0+x, sy=by0+y; int si=sy*ww+sx; int l=lab[si];
        if(l<0||!keep[l])continue;
        int ox=dstx+x, oy=dsty+y; if(ox<0||ox>=cw||oy<0||oy>=ch)continue;
        int di=(oy*cw+ox)*4, s4=si*4;
        op[di]=col4[s4];op[di+1]=col4[s4+1];op[di+2]=col4[s4+2];op[di+3]=col4[s4+3];}
    }
    Marshal.Copy(op,0,od.Scan0,op.Length); outbmp.UnlockBits(od);
    outbmp.Save(dst,ImageFormat.Png); outbmp.Dispose();
    return (tile?"tile":"recenter")+" comps="+nl+" kept="+kept;
  }
}
"@

$repo = "C:\Users\Charl\Desktop\SCPS-main\godot\project\assets\scps\ui\parch"
$roots = @(
  @{ dir="C:\Users\Charl\Documents\Codex\2026-06-29\an\outputs\scps_ui_parchment";                 chroma=$true  },
  @{ dir="C:\Users\Charl\Documents\Codex\2026-06-29\an\outputs\scps_ui_parchment_series2";         chroma=$false },
  @{ dir="C:\Users\Charl\Documents\Codex\2026-06-29\an\outputs\scps_ui_parchment_series3_final";   chroma=$true  },
  @{ dir="C:\Users\Charl\Documents\Codex\2026-06-29\an\outputs\scps_ui_parchment_series4_plateau"; chroma=$true  }
)
if($OutDir -ne "" -and -not (Test-Path $OutDir)){ New-Item -ItemType Directory -Force $OutDir | Out-Null }
$total=0
foreach($root in $roots){
  foreach($sd in (Get-ChildItem $root.dir -Directory | Where-Object {$_.Name -like 'sheet*' -and $_.Name -like $SheetFilter} | Sort-Object Name)){
    $mf = Join-Path $sd.FullName 'manifest.json'
    if(-not (Test-Path $mf)){ continue }
    $m = Get-Content $mf -Raw | ConvertFrom-Json
    $cols=[int]$m.grid[0]; $rows=[int]$m.grid[1]; $cw=[int]$m.cell_size[0]; $ch=[int]$m.cell_size[1]
    $sheetDir = Join-Path $sd.FullName 'sheet'
    $master = Get-ChildItem $sheetDir -Filter *chromakey*.png -ErrorAction SilentlyContinue | Select-Object -First 1
    $chroma = $root.chroma
    if(-not $master){ $master = Get-ChildItem $sheetDir -Filter *.png | Select-Object -First 1; $chroma=$false }
    $names = @(Get-ChildItem $repo -Filter "$($sd.Name)_*.png" | Sort-Object Name | Select-Object -ExpandProperty Name)
    if($names.Count -ne ($cols*$rows)){ Write-Host ("SKIP {0}: {1} repo files vs {2} grid cells" -f $sd.Name,$names.Count,($cols*$rows)); continue }
    [Recut]::Load($master.FullName, $chroma)
    for($i=0;$i -lt $names.Count;$i++){
      $r=[math]::Floor($i/$cols); $c=$i%$cols
      $dst = if($OutDir -ne ""){ Join-Path $OutDir $names[$i] } else { Join-Path $repo $names[$i] }
      [void][Recut]::Cell($c,$r,$cw,$ch,$Pad,$dst)
      $total++
    }
    Write-Host ("ok {0} ({1} cells)" -f $sd.Name,$names.Count)
  }
}
Write-Host "RECUT total: $total cells."
#pragma once
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include "display.h"

#define AP_SSID "keychain-reader"
AsyncWebServer server(80);

static const char UPLOAD_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html><html lang="en"><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Keychain Reader</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,sans-serif;background:#f5f5f0;color:#222;max-width:520px;margin:0 auto;padding:24px 16px}
h1{font-size:1.3rem;margin-bottom:4px}.sub{color:#666;font-size:.85rem;margin-bottom:24px}
#dropzone{border:2px dashed #bbb;border-radius:10px;padding:36px 20px;text-align:center;cursor:pointer;background:#fff;transition:all .2s}
#dropzone.hover{border-color:#333;background:#f0f0ec}
#dropzone input{display:none}.icon{font-size:2rem;margin-bottom:8px}
#dropzone p{font-size:.9rem;color:#555}#dropzone small{color:#999;font-size:.78rem}
#pbwrap{margin-top:12px;height:6px;background:#e0e0e0;border-radius:4px;overflow:hidden;display:none}
#pb{height:100%;background:#333;width:0;transition:width .3s}
#status{margin-top:8px;font-size:.85rem;color:#555;min-height:20px}
.stitle{font-size:.75rem;font-weight:600;text-transform:uppercase;letter-spacing:.08em;color:#888;margin:28px 0 10px}
#sbwrap{height:8px;background:#e0e0e0;border-radius:4px;overflow:hidden;margin-bottom:6px}
#sb{height:100%;background:#333;transition:width .4s}#slabel{font-size:.78rem;color:#888}
.fi{display:flex;justify-content:space-between;align-items:center;padding:10px 12px;background:#fff;border-radius:8px;margin-bottom:6px;border:1px solid #e8e8e5}
.fn{font-size:.88rem}.fs{font-size:.78rem;color:#999;margin-top:2px}
.db{background:none;border:none;cursor:pointer;color:#bbb;font-size:1.1rem;padding:4px 8px;border-radius:4px}
.db:hover{color:#c00}.empty{color:#aaa;font-size:.88rem;padding:16px 0;text-align:center}
</style></head><body>
<h1>📚 Keychain Reader</h1>
<p class="sub">Upload plain .txt books to your device</p>
<div id="dropzone" onclick="document.getElementById('fi').click()">
<input type="file" id="fi" accept=".txt" multiple>
<div class="icon">📄</div>
<p>Drop <strong>.txt</strong> files here</p>
<small>or click to browse &nbsp;·&nbsp; multiple files OK</small>
</div>
<div id="pbwrap"><div id="pb"></div></div>
<div id="status"></div>
<p class="stitle">Storage</p>
<div id="sbwrap"><div id="sb" style="width:0%"></div></div>
<div id="slabel">Loading...</div>
<p class="stitle">Books on Device</p>
<div id="filelist"></div>
<script>
const dz=document.getElementById('dropzone'),fi=document.getElementById('fi'),
st=document.getElementById('status'),pb=document.getElementById('pb'),
pbw=document.getElementById('pbwrap');
dz.addEventListener('dragover',e=>{e.preventDefault();dz.classList.add('hover')});
dz.addEventListener('dragleave',()=>dz.classList.remove('hover'));
dz.addEventListener('drop',e=>{e.preventDefault();dz.classList.remove('hover');upload(e.dataTransfer.files)});
fi.addEventListener('change',()=>upload(fi.files));
async function upload(files){for(const f of files)await uploadOne(f);loadFiles()}
function uploadOne(file){
  return new Promise(r=>{
    const fd=new FormData();fd.append('file',file,file.name);
    st.textContent='Uploading '+file.name+'...';pbw.style.display='block';pb.style.width='0%';
    const x=new XMLHttpRequest();x.open('POST','/upload');
    x.upload.onprogress=e=>{if(e.lengthComputable){const p=Math.round(e.loaded/e.total*100);pb.style.width=p+'%';st.textContent='Uploading '+file.name+'... '+p+'%'}};
    x.onload=()=>{st.textContent='✅ '+file.name+' uploaded!';setTimeout(()=>{pbw.style.display='none';r()},600)};
    x.onerror=()=>{st.textContent='❌ Failed: '+file.name;r()};x.send(fd);
  });
}
function fmt(b){if(b<1024)return b+' B';if(b<1048576)return(b/1024).toFixed(1)+' KB';return(b/1048576).toFixed(2)+' MB'}
function loadFiles(){
  fetch('/files').then(r=>r.json()).then(d=>{
    const list=document.getElementById('filelist'),label=document.getElementById('slabel'),bar=document.getElementById('sb');
    const p=Math.round(d.used/d.total*100);bar.style.width=p+'%';
    label.textContent=fmt(d.used)+' used of '+fmt(d.total)+' ('+p+'%)';
    list.innerHTML='';
    if(!d.files.length){list.innerHTML='<p class="empty">No books yet</p>';return}
    d.files.forEach(f=>{
      const el=document.createElement('div');el.className='fi';
      el.innerHTML='<div><div class="fn">'+f.name+'</div><div class="fs">'+fmt(f.size)+'</div></div><button class="db" onclick="del(\''+f.name+'\')">🗑</button>';
      list.appendChild(el);
    });
  });
}
function del(name){if(!confirm('Delete '+name+'?'))return;fetch('/delete?file='+encodeURIComponent(name)).then(()=>loadFiles())}
loadFiles();
</script></body></html>
)rawhtml";

void startUploadServer() {
  WiFi.softAP(AP_SSID);
  Serial.println("AP: keychain-reader @ 192.168.4.1");
  displayMessage("Upload Mode", "WiFi: keychain-reader", "-> 192.168.4.1");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req){
    req->send_P(200, "text/html", UPLOAD_HTML);
  });
  server.onNotFound([](AsyncWebServerRequest* req){
    req->redirect("http://192.168.4.1/");
  });
  server.on("/upload", HTTP_POST,
    [](AsyncWebServerRequest* req){ req->send(200, "text/plain", "OK"); },
    [](AsyncWebServerRequest* req, const String& filename, size_t index, uint8_t* data, size_t len, bool final){
      String path = "/" + filename;
      int ls = path.lastIndexOf('/'); if(ls > 0) path = "/" + path.substring(ls+1);
      if(index == 0){
        if(LittleFS.exists(path)) LittleFS.remove(path);
        req->_tempFile = LittleFS.open(path, FILE_WRITE);
      }
      if(req->_tempFile) req->_tempFile.write(data, len);
      if(final && req->_tempFile) req->_tempFile.close();
    }
  );
  server.on("/files", HTTP_GET, [](AsyncWebServerRequest* req){
    String json = "{\"files\":["; bool first = true;
    File root = LittleFS.open("/"); File f = root.openNextFile();
    while(f){
      String name = String(f.name());
      if(!name.startsWith("/")) name = "/" + name;
      if(name.endsWith(".txt") && name != "/bookmark.txt"){
        if(!first) json += ",";
        json += "{\"name\":\""+name.substring(1)+"\",\"size\":"+f.size()+"}";
        first = false;
      }
      f = root.openNextFile();
    }
    json += "],\"used\":"+String(LittleFS.usedBytes())+",\"total\":"+String(LittleFS.totalBytes())+"}";
    req->send(200, "application/json", json);
  });
  server.on("/delete", HTTP_GET, [](AsyncWebServerRequest* req){
    if(req->hasParam("file")){
      String path = "/" + req->getParam("file")->value();
      if(path != "/bookmark.txt" && LittleFS.exists(path)) LittleFS.remove(path);
    }
    req->send(200, "text/plain", "OK");
  });
  server.begin();
}
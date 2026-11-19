export default {
  async fetch(request, env) {
    const url = new URL(request.url);

    const corsHeaders = {
      "Access-Control-Allow-Origin": "*",
      "Access-Control-Allow-Methods": "GET, POST, OPTIONS",
      "Access-Control-Allow-Headers": "Content-Type",
    };

    if (request.method === "OPTIONS") {
      return new Response(null, { headers: corsHeaders });
    }

    // Endpoint for laptop to push its encrypted / triangulated location
    if (url.pathname === "/api/update" && request.method === "POST") {
      try {
        const body = await request.json();
        body.updated_at = new Date().toISOString();
        if (env.TRACKER_KV) {
          await env.TRACKER_KV.put("latest_location", JSON.stringify(body));
        }
        return new Response(JSON.stringify({ status: "ok" }), {
          headers: { ...corsHeaders, "Content-Type": "application/json" }
        });
      } catch (err) {
        return new Response(JSON.stringify({ error: err.message }), { status: 400, headers: corsHeaders });
      }
    }

    // Endpoint for your phone to read the latest location
    if (url.pathname === "/api/location") {
      let data = null;
      if (env.TRACKER_KV) {
        data = await env.TRACKER_KV.get("latest_location");
      }
      return new Response(data || JSON.stringify({ error: "No location reported yet" }), {
        headers: { ...corsHeaders, "Content-Type": "application/json" }
      });
    }

    // Direct Phone Map View (OpenStreetMap Leaflet, zero API keys, 100% clean)
    if (url.pathname === "/" || url.pathname === "/map") {
      const html = `<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>FindMy Linux — Live Radar</title>
  <link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css" />
  <style>
    body, html { margin: 0; padding: 0; height: 100%; background: #0f111a; font-family: -apple-system, sans-serif; color: #fff; }
    #map { height: 100%; width: 100%; }
    .status-bar {
      position: absolute; top: 12px; left: 12px; right: 12px; z-index: 1000;
      background: rgba(22, 24, 38, 0.92); backdrop-filter: blur(12px);
      padding: 14px 18px; border-radius: 16px; border: 1px solid rgba(255,255,255,0.1);
      box-shadow: 0 8px 32px rgba(0,0,0,0.4);
    }
    .status-title { font-size: 16px; font-weight: 700; display: flex; align-items: center; gap: 8px; }
    .pulse-dot { width: 8px; height: 8px; background: #10b981; border-radius: 50%; box-shadow: 0 0 8px #10b981; }
    .status-sub { font-size: 13px; color: #9ca3af; margin-top: 4px; font-family: monospace; }
    .nav-btn {
      display: inline-block; margin-top: 10px; background: #3b82f6; color: #fff;
      padding: 8px 14px; border-radius: 8px; text-decoration: none; font-size: 13px; font-weight: 600;
    }
  </style>
</head>
<body>
  <div class="status-bar">
    <div class="status-title"><div class="pulse-dot"></div> ASUS Vivobook S 15</div>
    <div class="status-sub" id="coords">Waiting for device telemetry...</div>
    <a id="nav-link" class="nav-btn" href="#" target="_blank" style="display:none;">Open in Google Maps</a>
  </div>
  <div id="map"></div>
  <script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css"></script>
  <script>
    const map = L.map('map', { zoomControl: false }).setView([28.6519, 77.2315], 14);
    L.tileLayer('https://tile.openstreetmap.org/{z}/{x}/{y}.png', { maxZoom: 19 }).addTo(map);
    let marker, circle;
    async function update() {
      try {
        const res = await fetch('/api/location');
        const d = await res.json();
        if (d.latitude && d.longitude) {
          document.getElementById('coords').innerText = d.latitude.toFixed(6) + ', ' + d.longitude.toFixed(6) + ' (±' + (d.accuracy||15) + 'm) • ' + (d.city||'');
          const nav = document.getElementById('nav-link');
          nav.href = 'https://www.google.com/maps?q=' + d.latitude + ',' + d.longitude;
          nav.style.display = 'inline-block';
          if (!marker) {
            marker = L.marker([d.latitude, d.longitude]).addTo(map);
            circle = L.circle([d.latitude, d.longitude], { radius: d.accuracy || 25, color: '#3b82f6', fillColor: '#3b82f6', fillOpacity: 0.2 }).addTo(map);
            map.setView([d.latitude, d.longitude], 16);
          } else {
            marker.setLatLng([d.latitude, d.longitude]);
            circle.setLatLng([d.latitude, d.longitude]);
          }
        }
      } catch(e){}
    }
    update();
    setInterval(update, 5000);
  </script>
</body>
</html>`;
      return new Response(html, { headers: { "Content-Type": "text/html" } });
    }

    return new Response("Not Found", { status: 404 });
  }
};

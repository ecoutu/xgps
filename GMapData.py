# GMapData module

# Andrew Berry, CIS 2750 A3 Prototype
# February 14, 2007
# W Gardner, repackaged as module with OO
# "EZ" edition, doesn't use RPC, but outputs HTML/Javascript directly
# February 20, 2007
# W Gardner, allow browser to be closed w/o "no running window found" and
# "no such process" errors.
# February 20, 2010

import os, signal, time
import CGIHTTPServer, BaseHTTPServer

### Global variables

serverPids = [0,0]     # list of server process IDs [web server,browser]


### Public functions

# Start CGI/HTML web server (do once).  Pages to be served must be in
# public_html.
#
# portNum: port number that HTTP server should listen on. Recommend 4xxxx,
#   where "xxxx" are the last 4 digits of your student number. 
#
def startWebServer(portNum=8080):
    pid = os.fork()
    serverPids[0] = pid
    if (pid == 0):            # in child process
        # The website is served from public_html, and CGI programs
        # of any type (perl, python, etc) can be placed in cgi-bin
        class Handler(CGIHTTPServer.CGIHTTPRequestHandler):
            cgi_directories = ["/cgi-bin"]

        os.chdir('public_html');
        httpd = BaseHTTPServer.HTTPServer(("", portNum), Handler)
        print "Web server on port:", portNum
        httpd.serve_forever()
        

# Kill all servers
#
def killServers():
    print "Killing all servers...",serverPids
    for pid in serverPids:
        try:
            print " Sending SIGHUP to ",pid
            os.kill(pid, signal.SIGHUP)
        except OSError:
            print "OS Error: PID " + str(pid) + " has already died."

# Launch Firefox browser (add tab to browser)
#
# url: URL to be opened in Firefox browser, "http://localhost:8080/" (replace
#   8080 with the port number of the web server).
# options: as list (not string); see "man firefox"
#
def launchBrowser(url, options=[]):
    FIREFOX = "/usr/bin/firefox"
    if (serverPids[1] == 0 ):        # open first browser instance
        pid = os.fork()
        serverPids[1] = pid
        if (pid == 0):            # in child process
            os.execv(FIREFOX, [FIREFOX]+options+[url]);
    else:                            # open new tab in existing browser
        pid = os.fork()
        if (pid == 0):            # in child process
            os.execl(FIREFOX, FIREFOX, url);
        else:
            os.wait()

def serve(path):
    htm = open(path, 'w')

    print >> htm, """
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
 <head>
  <meta http-equiv="content-type" content="text/html; charset=utf-8"/>
  <title>xpgs</title>
  <script src="http://maps.google.com/maps?file=api&amp;v=2&amp;key=ABQIAAAASveZAKDchy8aE-6K6klrVRTwM0brOpm-All5BF6PoaKBxRWWERS4kZrUi1p4QOZXGVxTLHvCXsg94A"
          type="text/javascript"></script>
  <script type="text/javascript">
  //<![CDATA[
  function load() {
    if (GBrowserIsCompatible()) {
      var baseIcon = new GIcon();
      baseIcon.shadow = "http://labs.google.com/ridefinder/images/mm_20_shadow.png"
      baseIcon.iconSize = new GSize(18, 30);
      baseIcon.shadowSize = new GSize(33, 30);
      baseIcon.iconAnchor = new GPoint(9, 30);
      baseIcon.infoWindowAnchor = new GPoint(5, 1);
      var icons = [new GIcon(baseIcon),new GIcon(baseIcon),new GIcon(baseIcon)];
      var map = new GMap2(document.getElementById("map"));
      map.setCenter(new GLatLng(0, 0), 0);
      map.enableScrollWheelZoom()
      map.addControl(new GLargeMapControl());
      map.addControl(new GMapTypeControl());
      var bounds = new GLatLngBounds();

      function coord(lat, lon) {
        c = new GLatLng(lat, lon);
        bounds.extend(c);
        return c;
      }

      function icon(hexcolor, symbol) {
        var color = hexcolor.replace('#', '');
        var gicon = new GIcon(baseIcon);
        var iconurl = 'http://chart.apis.google.com/chart?chst=d_map_pin_icon&chld='
        var icons = {
          'flag': ['flag'], 
          'bus': ['bus'],
          'airport': ['airport', 'plane'],
          'bicycle': ['bicycle', 'bike'],
          'car': ['car', 'automobile'],
          'caution': ['caution', 'hazard', 'skull'],
          'home': ['home'],
          'info': ['info'],
          'parking': ['parking'],
          'petrol': ['petrol', 'gas', 'fuel'],
          'sail': ['boat', 'sail'],
          'taxi': ['taxi', 'cab'], 
          'academy': ['school', 'university'],
          'computer': ['computer'],
          'beer': ['beer'],
          'restaurant': ['food'],
          'cafe': ['cafe'],
          'bank-dollar': ['money'],
          'shoppingbag': ['shoppingbag'],
          'medical': ['medical'],
          'ski': ['ski']
        };
        if (symbol) {
          for (key in icons) {
            for (var i = 0; i < icons[key].length; i++) {
              if (symbol.toLowerCase() == icons[key][i]) {
                gicon.image =  iconurl + key.toLowerCase() + '|' + color;
                return gicon;
              }
            }
          }
        }
        gicon.image =  'http://chart.apis.google.com/chart?chst=d_map_pin_letter&chld=|' + color + '|000000';
        return gicon;
      }

      function marker(c, title, icon, info) {
        var mark = new GMarker(c, {icon:icon, title:title});
        if (info) {
          GEvent.addListener(mark, "click", function() {
            mark.openInfoWindowHtml(info);
          });
        }
        return mark;
      }

      function polyline(points, color, text) {
        var line = new GPolyline(points, color);
        GEvent.addListener(line, "click", function(latlng) {
          map.openInfoWindowHtml(latlng, text);
        });
        GEvent.addListener(line, "mouseover", function() {
          line.setStrokeStyle({weight: 10})
        });
        GEvent.addListener(line, "mouseout", function() {
          line.setStrokeStyle({weight: 5})
        });
        return line;
      }
      GDownloadUrl("gmapdata.xml", function(doc) {
        var xml = GXml.parse(doc);

        var waypts = xml.documentElement.getElementsByTagName("waypt");
        
        for (var i = 0; i < waypts.length; i++) {
          var lat = parseFloat(waypts[i].getAttribute("lat"));
          var lon = parseFloat(waypts[i].getAttribute("lon"));
          var id = waypts[i].getAttribute("id");
          var color = waypts[i].getAttribute("color");
          var symbol = waypts[i].getAttribute("symbol");
          var text = waypts[i].getAttribute("text");

          map.addOverlay(marker(coord(lat, lon), id, icon(color, symbol), text));
        }
        
        var routes = xml.documentElement.getElementsByTagName("route");
        for (var i = 0; i < routes.length; i++) {
          var legs = [];
          var color = routes[i].getAttribute("color");
          var text = routes[i].getAttribute("comment");
          var num = routes[i].getAttribute("num");
          for (var j = 0; j < routes[i].childNodes.length; j++) {
            var lat = parseFloat(routes[i].childNodes[j].getAttribute("lat"));
            var lon = parseFloat(routes[i].childNodes[j].getAttribute("lon"));
            legs.push(coord(lat, lon));
          }
          map.addOverlay(polyline(legs, color, "<em style='font-size: x-small'>Route</em> - <b>No. " + num + "</b><br />" + text));
        }
        
        var trkpts = xml.documentElement.getElementsByTagName("trkpt");
        for (var i = 0; i < trkpts.length; i++) {
          var color = trkpts[i].getAttribute("color");
          var lat = parseFloat(trkpts[i].getAttribute("lat"));
          var lon = parseFloat(trkpts[i].getAttribute("lon"));
          map.addOverlay(marker(coord(lat, lon), i, icon(color)));
        }
        
        var tracks = xml.documentElement.getElementsByTagName("track");
        for (var i = 0; i < tracks.length; i++) {
          var legs = [];
          var color = tracks[i].getAttribute("color");
          var dist = tracks[i].getAttribute("dist");
          var duration = tracks[i].getAttribute("duration");
          var seqno = tracks[i].getAttribute("seqno");
          var speed = tracks[i].getAttribute("speed");
          var start = tracks[1].getAttribute("start");
          for (var j = 0; j < tracks[i].childNodes.length; j++) {
            var lat = parseFloat(tracks[i].childNodes[j].getAttribute("lat"));
            var lon = parseFloat(tracks[i].childNodes[j].getAttribute("lon"));
            legs.push(coord(lat, lon));
          }
          map.addOverlay(polyline(legs, color, "<em style='font-size: x-small'>Track</em> - <b>Sequence No. " + seqno + "</b><br />Starts: " + start + "<br />Duration: " + duration + "<br />Distance: " + dist + "<br />Speed: " + speed));
        }
        map.setZoom(map.getBoundsZoomLevel(bounds));
        map.setCenter(bounds.getCenter());
      });
    }
  }
  //]]>
  </script>
 </head>
 <body onload="load()" onunload="GUnload()">
  <div id="map" style="position:absolute;top:0px;left:5px;width:100%;height:100%"></div>
 </body>
</html>"""
    htm.close()
    launchBrowser('http://localhost:43365')


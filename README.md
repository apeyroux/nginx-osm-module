nginx-osm-module
================

NOTE : This module does not work. It is only experimentation nginx module.

To install, compile nginx with this ./configure option:

    --add-module=path/to/this/directory

My nginx.conf has this:

    location /osm {
        osm;
        nb_mapnik_th 8;
    }

TODO/IDEAS
==========

1. If loadav > x , use reverse on tile.osm.org
2. http://mysrv/osm/0.0.0.json -> return json tile (count, stat, date, geo (bbox) for stat, size ...)

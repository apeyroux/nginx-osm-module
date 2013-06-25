nginx-osm-module
================

NOTE : This module does not work. It is only experimentation nginx module.
---

To install, compile nginx with this ./configure option:

    --add-module=path/to/this/directory

My nginx.conf has this:

    location /osm {
        osm;
        nb_mapnik_th 8;
    }

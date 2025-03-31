#pragma once

#include <cstdint>

enum JpegMarkers { SOI, SOF0, SOF2, DHT, DQT, DRI, SOS, RSTn, APPn, COM, EOI, NOTAMARKER };

JpegMarkers IdentMarker(uint16_t bytes) {
    if (bytes == 0xffd8) {
        return JpegMarkers::SOI;
    }
    if (bytes == 0xffc0) {
        return JpegMarkers::SOF0;
    }
    if (bytes == 0xffc2) {
        return JpegMarkers::SOF2;
    }
    if (bytes == 0xffc4) {
        return JpegMarkers::DHT;
    }
    if (bytes == 0xffdb) {
        return JpegMarkers::DQT;
    }
    if (bytes == 0xffdd) {
        return JpegMarkers::DRI;
    }
    if (bytes == 0xffda) {
        return JpegMarkers::SOS;
    }
    if (0xffd0 <= bytes && bytes <= 0xffd7) {
        return JpegMarkers::RSTn;
    }
    if (0xffe0 <= bytes && bytes <= 0xffef) {
        return JpegMarkers::APPn;
    }
    if (bytes == 0xfffe) {
        return JpegMarkers::COM;
    }
    if (bytes == 0xffd9) {
        return JpegMarkers::EOI;
    }
    return JpegMarkers::NOTAMARKER;
}
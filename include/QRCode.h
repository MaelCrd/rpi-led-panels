#ifndef QRCODE_H
#define QRCODE_H

#include <string>
#include <vector>

#include "qrcodegen.hpp"

void inline generateQR(std::string &text, std::vector<std::vector<bool>> &qr) {
  // Manual operation
  auto segs = qrcodegen::QrSegment::makeSegments(text.c_str());
  auto qr1 = qrcodegen::QrCode::encodeSegments(
      segs, qrcodegen::QrCode::Ecc::HIGH, 5, 5, 2, false);
  qr.resize(qr1.getSize(), std::vector<bool>(qr1.getSize(), false));
  for (int y = 0; y < qr1.getSize(); y++) {
    for (int x = 0; x < qr1.getSize(); x++) {
      qr[y][x] = qr1.getModule(x, y);
    }
  }
}

#endif // QRCODE_H
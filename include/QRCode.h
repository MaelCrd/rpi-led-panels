#ifndef QRCODE_H
#define QRCODE_H

// import
#include "qrcodegen.hpp"

using namespace qrcodegen;

// Your QR code generation and manipulation functions go here

void generateQR(std::string &text, std::vector<std::vector<bool>> &qr) {
  // Manual operation
  std::vector<QrSegment> segs = QrSegment::makeSegments(text.c_str());
  QrCode qr1 = QrCode::encodeSegments(segs, QrCode::Ecc::HIGH, 5, 5, 2, false);
  qr.resize(qr1.getSize(), std::vector<bool>(qr1.getSize(), false));
  for (int y = 0; y < qr1.getSize(); y++) {
    for (int x = 0; x < qr1.getSize(); x++) {
      qr[y][x] = qr1.getModule(x, y);
    }
  }
}

#endif // QRCODE_H
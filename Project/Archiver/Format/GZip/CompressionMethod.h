// CompressionMethod.h

#pragma once

#ifndef __GZIP_COMPRESSIONMETHOD_H
#define __GZIP_COMPRESSIONMETHOD_H

namespace NArchive {
namespace NGZip {

struct CCompressionMethodMode
{
  UINT32 NumPasses;
  UINT32 NumFastBytes;
};

}}

#endif

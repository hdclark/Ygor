#include "miniz.c"

#include <memory>
#include <string>

#include "YgorMisc.h"
#include "YgorFilesDirs.h"
#include "YgorPerformance.h"


int main(int argc, char **argv){
    
    if(argc != 2) FUNCERR("Need to provide a filename to test compression/decompression");
    int ret = 0;

    long int size = 0;
    std::unique_ptr<Byte> raw = Load_Binary_File<Byte>(argv[1], &size);
    FUNCINFO("Raw data has size " << size);

    Byte *enc = new Byte [compressBound( (uLong)(size) )];
    uLong encsz = compressBound((uLong)(size));
    FUNCINFO("Empty buff has sz " << encsz);

    //Returns Z_OK on success, or one of the error codes from deflate() on failure.
    const double t0 = YgorPerformance_Get_Time();
    ret = compress(enc, &encsz,   reinterpret_cast<Byte*>(raw.get()), (uLong)(size));
    const double t1 = YgorPerformance_Get_Time();
    if(ret != Z_OK) FUNCERR("compress: did not find Z_OK");
    FUNCINFO("Enc data has size " << encsz << ", or " << encsz*100.0/size << "%. Compression rate: " << (double)(size)/(1024*1024*(t1-t0)) << " MB/s");

    // Like compress() but with more control, level may range from 0 (storing) to 9 (max. compression)
    //  int compress2(Byte *pDest, uLong *pDest_len, const Byte *pSource, uLong source_len, int level);

    Byte *dec = new Byte [2*size];
    uLong decsz = (uLong)(2*size);


    // Returns Z_OK on success, or one of the error codes from inflate() on failure.
    const double t2 = YgorPerformance_Get_Time();
    ret = uncompress(dec, &decsz, enc, encsz);
    const double t3 = YgorPerformance_Get_Time();
    if(ret != Z_OK) FUNCERR("decompress: did not find Z_OK");
    FUNCINFO("Decompression rate: " << (double)(size)/(1024*1024*(t3-t2)) << " MB/s");


    //Verify integrity.
    if((long int)(decsz) != size) FUNCERR("Original data and decompressed data are of different lengths");
    for(long int i = 0; i < size; ++i){
        if((raw.get())[i] != dec[i]) FUNCERR("Original data and decompressed data vary in byte " << i);
    }


    return 0;
}

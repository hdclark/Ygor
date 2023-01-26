//Test_Serialization.cc.

#include <string>
#include <list>
#include <vector>
#include <memory>

#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorSerialize.h"


int main(int argc, char **argv){
    {
      uint64_t max = 20*sizeof(uint8_t);
      std::unique_ptr<uint8_t[]> mem_chars( new uint8_t[max] );
      uint64_t offset = 0;
      bool OK = 0;

      //Write some numbers to the memory block.
      while(offset < max){
          const auto aval = static_cast<uint8_t>(offset);
          mem_chars = SERIALIZE::Put<uint8_t>(&OK, std::move(mem_chars), &offset, max, aval);
          if(OK == false) FUNCERR("Encountered an error. Test failed");
      }

      //Read the numbers back.
      offset = 0;
      while(offset < max){
          uint8_t aval;
          mem_chars = SERIALIZE::Get<uint8_t>(&OK, std::move(mem_chars), &offset, max, &aval);
          if(OK == false) FUNCERR("Encountered an error. Test failed");
          std::cout << "Read back integer '" << (int)(aval) << "'" << std::endl;
      }
    }


    {
      uint64_t max = 20*sizeof(double);
      std::unique_ptr<uint8_t[]> mem_dbls( new uint8_t[max] );
      uint64_t offset = 0;
      bool OK = 0;

      //Write some numbers to the memory block.
      while(offset < max){
          const auto aval = static_cast<double>(offset);
          mem_dbls = SERIALIZE::Put<double>(&OK, std::move(mem_dbls), &offset, max, aval);
          if(OK == false) FUNCERR("Encountered an error. Test failed");
      }

      //Read the numbers back.
      offset = 0;
      while(offset < max){
          double aval;
          mem_dbls = SERIALIZE::Get<double>(&OK, std::move(mem_dbls), &offset, max, &aval);
          if(OK == false) FUNCERR("Encountered an error. Test failed");
          std::cout << "Read back double '" << aval << "'" << std::endl;
      }
    }

    {
      uint64_t max = 5000;
      std::unique_ptr<uint8_t[]> mem_blk( new uint8_t[max] );
      uint64_t offset = 0;
      bool OK = 0;

      std::basic_string<char> teststr("This is my test string. I hope it is saved and recovered correctly.");

      //Write the string to the memory block.
      mem_blk = SERIALIZE::Put_String<char>(&OK, std::move(mem_blk), &offset, max, teststr);
      if(OK == false) FUNCERR("Encountered an error. Test failed");

      //Prepare for the read back.
      teststr.clear();
      teststr = "If you see this, uh-oh!";
      uint64_t actualmax = offset;
      offset = 0;

      //Read the string back from the buffer.
      mem_blk = SERIALIZE::Get_String<char>(&OK, std::move(mem_blk), &offset, actualmax, &teststr);
      if(OK == false) FUNCERR("Encountered an error. Test failed");

      std::cout << "Read back string '" << teststr << "'" << std::endl;
    }



    {
      uint64_t max = 5000;
      std::unique_ptr<uint8_t[]> mem_blk( new uint8_t[max] );
      uint64_t offset = 0;
      bool OK = 0;

      std::list<double> testlist({1.0, 2.0, 3.0, 4.0, 5.0});

      //Write the string to the memory block.
      mem_blk = SERIALIZE::Put_Sequence_Container(&OK, std::move(mem_blk), &offset, max, testlist);
      if(OK == false) FUNCERR("Encountered an error. Test failed");

      //Prepare for the read back.
      testlist.clear();
      uint64_t actualmax = offset;
      offset = 0;

      //Read the string back from the buffer.
      mem_blk = SERIALIZE::Get_Sequence_Container(&OK, std::move(mem_blk), &offset, actualmax, &testlist);
      if(OK == false) FUNCERR("Encountered an error. Test failed");

      std::cout << "The list of numbers stored was: ";
      for(auto it = testlist.begin(); it != testlist.end(); ++it){
          std::cout << *it << "    ";
      }
      std::cout << std::endl;
    }

    {
      uint64_t max = 5000;
      std::unique_ptr<uint8_t[]> mem_blk( new uint8_t[max] );
      uint64_t offset = 0;
      bool OK = 0;

      std::list<double> testlist({1.0, 2.0, 3.0, 4.0, 5.0});

      //Write the string to the memory block.
      mem_blk = SERIALIZE::Put_Sequence_Container(&OK, std::move(mem_blk), &offset, max, testlist);
      if(OK == false) FUNCERR("Encountered an error. Test failed");

      //Prepare for the read back.
      testlist.clear();
      std::vector<double> testvec;
      uint64_t actualmax = offset;
      offset = 0;

      //Read the string back from the buffer.
      mem_blk = SERIALIZE::Get_Sequence_Container(&OK, std::move(mem_blk), &offset, actualmax, &testvec);
      if(OK == false) FUNCERR("Encountered an error. Test failed");

      std::cout << "The vec of numbers (converted from the list of numbers) stored was: ";
      for(auto it = testvec.begin(); it != testvec.end(); ++it){
          std::cout << *it << "    ";
      }
      std::cout << std::endl;
    }




    return 0;
}

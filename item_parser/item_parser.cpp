#include <cstdint>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <regex>

struct item_entry
{
   char item[4];
   char padding;
   char size;
   uint16_t itemid;
   char wat[56];
   std::string name;
   std::string description;
};

struct table_struct
{
   uint16_t count;
   std::vector<item_entry> entries;
};

// https://stackoverflow.com/a/3824338
template <class T>
void endswap(T *objp)
{
   unsigned char *memp = reinterpret_cast<unsigned char*>(objp);
   std::reverse(memp, memp + sizeof(T));
}

void replaceStr(std::string& in, const std::string& find, const std::string& replace)
{
   auto pos = 0;
   while ((pos = in.find(find, pos)) != std::string::npos)
   {
      in.replace(pos, find.length(), replace);
      pos += replace.length();
   }
}

int main(int argc, char* argv[])
{
   std::regex alphaNumeric("[a-zA-Z]|[\u3000-\u303F]|[\u3040-\u309F]|[\u30A0-\u30FF]|[\uFF00-\uFFEF]|[\u4E00-\u9FAF]|[\u2605-\u2606]|[\u2190-\u2195]|[\u203B]");
   if (argc > 1)
   {
      std::string filePath(argv[1]);
      std::ifstream table;
      table.open(filePath, std::ios::binary | std::ios::ate);

      if (table.good())
      {
         auto size = table.tellg();
         table.seekg(0, std::ios::beg);
         std::vector<char> buf(size);

         if (table.read(buf.data(), size))
         {
            table.close();
            auto skipBytes = 0;

            std::ofstream outCsv("items.csv");
            outCsv.clear();
            outCsv.write("itemid, name, description\r\n", sizeof("itemid, name, description\r\n"));

            for (auto i = 2; i < size; ++i)
            {
               if (skipBytes-- > 0)
                  continue;

               auto offset = 0;
               item_entry entry;
               skipBytes = 6;
               
               memcpy(&entry.item[0], buf.data() + i, 4);
               offset = 5;
               skipBytes += *reinterpret_cast<uint8_t*>(buf.data() + i + offset);
               offset += 2;
               //endswap(reinterpret_cast<uint16_t*>(buf.data() + i + offset));
               entry.itemid = *reinterpret_cast<uint16_t*>(buf.data() + i + offset);
               offset += 2;
               memcpy(&entry.wat[0], buf.data() + i + offset, sizeof(entry.wat));
               offset += 56;

               uint8_t ignoreJump = 0;
               // this is incredibly retarded but seems to work so whatever
               {
                  //if (static_cast<uint8_t>(entry.wat[0]) != 0xFD)
                  {
                     if (static_cast<uint8_t>(entry.wat[1]) != 0xFF)
                        ignoreJump = offset = (offset - 56) + static_cast<uint8_t>(entry.wat[2]) + 1;
                  }
               }

               while (*reinterpret_cast<uint8_t*>(buf.data() + i + offset) == 0xFF)
                  offset++;
               while (*reinterpret_cast<uint8_t*>(buf.data() + i + offset - 1) != 0xFF)
                  offset--;


               // this is incredibly retarded but seems to work so whatever
               {
                  entry.name = std::string(buf.data() + i + offset);

                  while (entry.name.size() < 4 || entry.name == "item" || (!std::regex_match(entry.name.substr(0, 1), alphaNumeric) && ignoreJump < skipBytes))
                     entry.name = std::string(buf.data() + i + (offset = ignoreJump++));
               }
               offset += entry.name.size() + 1;
               entry.description = std::string(buf.data() + i + offset);
               entry.description = entry.description.size() > 4 ? entry.description : "";
               offset += entry.description.size();
               
               for (auto it = entry.description.begin(); it != entry.description.end(); ++it)
                  if (*it == '\r' || *it == '\n')
                     *it = ' ';
                  else if (*it == ',')
                     *it = '-';

               replaceStr(entry.description, ",", "--");
               replaceStr(entry.name, ",", "--");

               std::stringstream ss;
               ss << "0x" << std::setw(4) << std::setfill('0') << std::uppercase << std::hex << entry.itemid;

               std::string outStr(ss.str() + ", " + entry.name + ", " + entry.description + "\r\n");
               //std::cout << "filePos " << std::to_string(i) << " skipBytes " << std::to_string(skipBytes) << " offset " << std::to_string(offset) << std::endl;
               //std::cout << outStr << std::endl;
               outCsv.write( outStr.c_str(), outStr.size());
            }
         }
      }
   }
   else
   {
      std::cout << "usage: path/to/data/text/dat_us/t_item.tbl";
   }
}
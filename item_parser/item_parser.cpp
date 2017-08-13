#include <cstdint>
#include <string>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

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
               entry.itemid = *reinterpret_cast<uint16_t*>(buf.data() + i + offset);
               offset += 2;
               memcpy(&entry.wat[0], buf.data() + i + offset, sizeof(entry.wat));
               offset += 56;

               while (*reinterpret_cast<uint8_t*>(buf.data() + i + offset) == 0xFF)
                  offset++;
               while (*reinterpret_cast<uint8_t*>(buf.data() + i + offset - 1) != 0xFF)
                  offset--;

               entry.name = std::string(buf.data() + i + offset);
               offset += entry.name.size() + 1;
               entry.description = std::string(buf.data() + i + offset);
               offset += entry.description.size();
               
               for (auto it = entry.description.begin(); it != entry.description.end(); ++it)
                  if (*it == '\r' || *it == '\n')
                     *it = ' ';
                  else if (*it == ',')
                     *it = '-';

               replaceStr(entry.description, ",", "--");
               replaceStr(entry.name, ",", "--");

               std::string outStr(std::to_string(entry.itemid) + ", " + entry.name + ", " + entry.description + "\r\n");
               std::cout << "filePos " << std::to_string(i) << " skipBytes " << std::to_string(skipBytes) << " offset " << std::to_string(offset) << std::endl;
               std::cout << outStr << std::endl;
               outCsv.write( outStr.c_str(), outStr.size());
            }
         }
      }
   }
   else
   {
      std::cout << "usage: itepath/to/data/text/dat_us/t_item.tbl";
   }
}
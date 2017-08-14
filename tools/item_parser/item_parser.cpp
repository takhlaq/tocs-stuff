#include <cstdint>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <regex>

struct TableHeader
{
   std::string type;
   uint16_t dataSize;
   uint16_t internalId;
};

struct ItemHeader
{
   std::string dataType;
   uint16_t unknown[23];
};

struct ItemEntry
{
   TableHeader entryHeader;
   uint16_t padding;
   ItemHeader itemHeader;
   std::string name;
   std::string description;
};

struct ItemTable
{
   uint16_t count;
   std::vector<ItemEntry> entries;
} data;

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
               ItemEntry entry;
               memset(&entry, 0, sizeof(entry));
               
               // table entry header
               {
                  entry.entryHeader.type = std::string(buf.data() + i + offset);
                  offset += entry.entryHeader.type.size() + 1;
                  // number of bytes til end of data (also copies in item id and padding)
                  memcpy(&entry.entryHeader.dataSize, buf.data() + i + offset, 2);
                  offset += 2;
               }

               std::stringstream ss;
               // itemid
               {
                  memcpy(&entry.entryHeader.internalId, buf.data() + i + offset, 2);
                  offset += 2;
                  ss << "0x" << std::setw(4) << std::setfill('0') << std::uppercase << std::hex << entry.entryHeader.internalId;
               }
               // padding
               {
                  memcpy(&entry.padding, buf.data() + i + offset, 2);
                  offset += 2;
               }
               // item header
               {
                  entry.itemHeader.dataType = std::string(buf.data() + i + offset);
                  offset += entry.itemHeader.dataType.size() + 1;
                  memcpy(&entry.itemHeader.unknown[0], buf.data() + i + offset, sizeof(entry.itemHeader.unknown));
                  offset += sizeof(entry.itemHeader.unknown);
               }

               entry.name = std::string(buf.data() + i + offset);
               entry.description = std::string(buf.data() + i + offset + entry.name.size() + 1);
               for (auto j = 0; j < entry.description.size(); ++j)
                  if (entry.description[j] == '\r' || entry.description[j] == '\n')
                     entry.description[j] = ' ';
                  else if (entry.description[j] == ',')
                     entry.description[j] = '-';

               replaceStr(entry.name, ",", "-");
               replaceStr(entry.description, ",", "-");

               std::string outStr(ss.str() + ", " + entry.name + ", " + entry.description + "\n");
               outCsv.write(outStr.c_str(), outStr.size());
               skipBytes = entry.entryHeader.dataSize + entry.entryHeader.type.size() + 2;
            }
         }
      }
   }
   else
   {
      std::cout << "usage: path/to/data/text/dat_us/t_item.tbl";
   }
}
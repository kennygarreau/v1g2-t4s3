"""Exercise actual DMA methods with an ownership-checking fake SPI queue."""
from pathlib import Path
import subprocess,tempfile
s=Path('src/LilyGo_AMOLED.cpp').read_text()
methods=s[s.index('void LilyGo_AMOLED::pushColorsDMA_v2'):s.index('// Push (aka write pixel)',s.index('void LilyGo_AMOLED::pushColorsDMA_v2'))]
prefix=r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <deque>
#include <cstdio>
using esp_err_t=int;
constexpr int ESP_OK=0,ESP_ERR_TIMEOUT=1,portMAX_DELAY=-1;
constexpr int SPI_TRANS_MODE_QIO=1,SPI_TRANS_VARIABLE_CMD=2,SPI_TRANS_VARIABLE_ADDR=4,SPI_TRANS_VARIABLE_DUMMY=8;
constexpr unsigned SEND_BUF_SIZE=16384;
struct spi_transaction_t {int flags,cmd,addr;const void *tx_buffer;size_t length;};
struct spi_transaction_ext_t {spi_transaction_t base;int command_bits,address_bits,dummy_bits;};
std::deque<spi_transaction_t*> queue;
bool complete=false;int failAt=-1,queues=0,waits=0;
int spi_device_queue_trans(int,spi_transaction_t*t,int){if(queues++==failAt)return 2;queue.push_back(t);return 0;}
int spi_device_get_trans_result(int,spi_transaction_t**r,int wait){++waits;if(!complete&&wait==0)return ESP_ERR_TIMEOUT;assert(!queue.empty());*r=queue.front();queue.pop_front();return 0;}
struct {template<class...T>void printf(const char*,T...){} } Serial;
#define log_e(...) ((void)0)
class LilyGo_AMOLED {
public:
 int spi=1;size_t dmaPending=0;bool cs=false;
 spi_transaction_ext_t dmaTransactions[17]{};
 void setCS(){assert(!cs);cs=true;} void clrCS(){cs=false;}
 void pushColorsDMA_v2(uint16_t*,uint32_t);
 bool pushColorsDMAAsync(uint16_t*,uint32_t);bool finishDMA(bool);
};
'''
suffix=r'''
int main(){
 uint16_t pixels[SEND_BUF_SIZE*3]{};LilyGo_AMOLED board;
 assert(board.pushColorsDMAAsync(pixels,SEND_BUF_SIZE*3));
 assert(queue.size()==3&&waits==0&&board.cs);
 for(int i=0;i<3;++i){assert(queue[i]==&board.dmaTransactions[i].base);assert(queue[i]->tx_buffer==pixels+i*SEND_BUF_SIZE);}
 assert(!board.finishDMA(false)&&board.cs&&queue.size()==3);
 complete=true;assert(board.finishDMA(false)&&queue.empty()&&!board.cs);
 failAt=queues+1;assert(!board.pushColorsDMAAsync(pixels,SEND_BUF_SIZE*3));
 assert(queue.empty()&&!board.cs&&board.dmaPending==0);
 failAt=-1;board.pushColorsDMA_v2(pixels,32);assert(queue.empty()&&!board.cs);
 puts("PASS: async queue ownership, pending buffer, completion and failed queue cleanup");
}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.cpp').write_text(prefix+methods+suffix)
 subprocess.run(['clang++','-std=c++17','-fsanitize=address,undefined',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)

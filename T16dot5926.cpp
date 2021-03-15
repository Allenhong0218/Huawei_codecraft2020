//优化了P2inv的建立过程，省略了多次查找和排序
#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <thread>
#include <atomic>
#include <unistd.h>
#include <chrono>   
using namespace std;
using namespace std::chrono;

// #define TEST
#define PRINT cout
#define MAXTHREADNUM 16
typedef unsigned int uint;
typedef long long intll;
typedef vector<uint> Path;


struct Node{
    uint to;
    uint money;
    Node(uint x,uint y){to=x;money=y;}
    bool operator<(const Node&node)const{
        return to < node.to;
    }
    bool operator==(const Node&node)const{
        return to == node.to;
    }
    
};

vector<vector<Node>> G;
vector<vector<Node>> Ginv;
vector<string> idsComma; //0...n to sorted id
unordered_map<uint,uint> idHash; 
vector<uint> inputs;
vector<uint> moneys;

vector<Path> chunkAns[MAXTHREADNUM];//最多可开启16线程
string ansString[8];
uint nodeCnt;
uint ansCnt;
uint lineCnt;
char* buf;//mmapped buffer

void mmapRead(string &testFile){
    uint fd = open(testFile.c_str(), O_RDONLY);
    uint test_data_size = lseek(fd, 0, SEEK_END);
    buf = (char*)mmap(NULL, test_data_size, PROT_READ, MAP_PRIVATE, fd, 0);
    uint test=0;
    lineCnt=0;
    uint tmp=test_data_size;
    bool find6050=false;
    while(tmp--){
        if(*buf==','){
            inputs.push_back(test);
            test = 0;
            ++buf;
            continue;
        }
        if(*buf=='\n'){//金额数据，初赛并不需要使用
            moneys.push_back(test);
            ++lineCnt;
            test=0;
            ++buf;
            continue;
        }
        if(*buf=='\r'){
            ++buf;
            continue;
        }  
        test *= 10;
        test += *buf-48;
        ++buf;
    }
#ifdef TEST
    PRINT<<"Data size: "<<test_data_size<<endl;
    PRINT<<lineCnt<<" Records in Total"<<endl;
#endif
}

void initGraph(bool initGinv){
    
    auto tmp=inputs;
    sort(tmp.begin(),tmp.end());
    tmp.erase(unique(tmp.begin(),tmp.end()),tmp.end());
    nodeCnt=0;
    for(uint &x:tmp){
        idsComma.push_back(to_string(x)+',');
        idHash[x]=nodeCnt++;
    }
#ifdef TEST
    PRINT<< nodeCnt <<"Nodes in Total"<<endl;
#endif
    uint sz=inputs.size();
    G=vector<vector<Node>>(nodeCnt);
    uint *inDegrees=new uint[nodeCnt];
    uint *outDegrees=new uint[nodeCnt];
    if(!initGinv){
        for(uint i=0;i<sz;i+=2){
            uint u=idHash[inputs[i]],v=idHash[inputs[i+1]];
            uint money = moneys[i/2];
            G[u].push_back(Node(v,money));
            ++inDegrees[v];
            ++outDegrees[u];
        }
    }
    else{
        Ginv=vector<vector<Node>>(nodeCnt);
        for(uint i=0;i<sz;i+=2){
            uint u=idHash[inputs[i]],v=idHash[inputs[i+1]];
            uint money = moneys[i/2];
            G[u].push_back(Node(v,money));
            Ginv[v].push_back(Node(u,money));
            ++inDegrees[v];
            ++outDegrees[u];
        }      
    }
    vector<uint>inRemove,outRemove;
    for(uint i=0;i<nodeCnt;i++){
        if(inDegrees[i]==0){
            inRemove.push_back(i);
        }
        if(outDegrees[i]==0){
            outRemove.push_back(i);
        }
    }
    for(auto &item : inRemove){
        for(auto &neighbour: G[item]){
            if(--inDegrees[neighbour.to]==0){
                inRemove.push_back(neighbour.to);
            }
        }
    }
    for(auto &item : outRemove){
        for(auto &neighbour: G[item]){
            if(--outDegrees[neighbour.to]==0){
                outRemove.push_back(neighbour.to);
            }
        }
    }
    for(uint i=0;i<nodeCnt;i++){
        if(inDegrees[i]==0){
            G[i].clear();
            Ginv[i].clear();
            continue;
        }
        if(outDegrees[i]==0){
            G[i].clear();
            continue;
        }
        sort(G[i].begin(),G[i].end());
        sort(Ginv[i].begin(),Ginv[i].end());
    }
}

inline bool check(uint x,uint y){
    return x<=5ll*y && y<=3ll*x;
}


inline void get3neighbour(Node &vi,vector<bool> &unset){
    auto &Gi=Ginv[vi.to];
    if(!Gi.empty()){
        auto vj =lower_bound(Gi.begin(),Gi.end(),vi);
        for(;vj!=Gi.end();++vj){
            auto mj = vj->money;
            unset[vj->to]=true;
            auto &Gj = Ginv[vj->to];
            if(!Gj.empty()){
                auto vk = lower_bound(Gj.begin(),Gj.end(),vi);                
                for(;vk!=Gj.end();++vk){
                        auto mk = vk->money;
                        if(check(mk,mj)){
                            unset[vk->to]=true;
                            auto &Gk = Ginv[vk->to];
                            if(!Gk.empty()){
                                auto vl = lower_bound(Gk.begin(),Gk.end(),vi);
                                for(;vl!=Gk.end();++vl){
                                    auto ml = vl->money;
                                    if(check(ml,mk))
                                        unset[vl->to]=true;
                                }
                            }
                        }
                }
            }

        }
    }

}



void step3dfs(uint headmoney,uint premoney,vector<bool> &visited,vector<bool> &neighbour,
              uint chunkID,vector<Path> &chunkPath,Node &head,uint cur,uint depth,vector<uint> &path){
    auto it=lower_bound(G[cur].begin(),G[cur].end(),head);//定位第一个大于或等于head的邻接节点位置
    auto Curend = G[cur].end();
    if(it==Curend){
        return;
    }
    
    visited[cur]=true;
    path.push_back(cur);
    if(it->to==head.to && check(premoney,it->money)&&check(it->money,headmoney)) {
        chunkPath.emplace_back(path);
    }
    if(depth<7){//DFS深度小于7
        for(;it!=Curend;++it){
            if( neighbour[it->to] && !visited[it->to] &&check(premoney,it->money)){
                step3dfs(headmoney,it->money,visited,neighbour,chunkID,chunkPath,head,it->to,depth+1,path);
            }
        }
    }
    visited[cur]=false;
    path.pop_back();
    return;
}

void step3cirChunk(uint chunkID,uint idxBegin,uint idxEnd){
    //寻找chunk中的环
#ifdef TEST
    auto start = system_clock::now();
#endif
    for(uint i=idxBegin;i<idxEnd;i++){

        if(!G[i].empty()){//i->j->k->l->m->n->o->i
            vector<bool>visited(nodeCnt,false);
            vector<uint> path;
            path.push_back(i);//head
            visited[i]=true;
            vector<bool>neighbour(nodeCnt,false);
            auto headNode = Node(i,0);
            get3neighbour(headNode,neighbour);
            auto vj = lower_bound(G[i].begin(),G[i].end(),headNode);
            for(;vj!=G[i].end();++vj){
                if(!visited[vj->to] && !G[vj->to].empty()){
                    path.push_back(vj->to);//i,j
                    visited[vj->to]=true;
                    auto mj = vj->money;
                    auto vk = lower_bound(G[vj->to].begin(),G[vj->to].end(),headNode);
                    for(;vk!=G[vj->to].end();++vk){
                        auto mk = vk->money;
                        if(!visited[vk->to] && !G[vk->to].empty() && check(mj,mk)){
                            path.push_back(vk->to);//i,j,k
                            visited[vk->to]=true;
                            auto vl = lower_bound(G[vk->to].begin(),G[vk->to].end(),headNode);
                            
                            if( vl!=G[vk->to].end() && vl->to==i && 
                                check(mk,vl->money) &&
                                check(vl->money,mj)){//find cylcle,depth=3
                                chunkAns[chunkID].emplace_back(path);
                            }
                            for(;vl!=G[vk->to].end();++vl){//
                                if(!visited[vl->to] && !G[vl->to].empty() && check(mk,vl->money)){
                                    path.push_back(vl->to);//i,j,k,l,path长为4
                                    visited[vl->to]=true;
                                    auto vm = lower_bound(G[vl->to].begin(),G[vl->to].end(),headNode);
                                    if(vm != G[vl->to].end() && vm->to == i && 
                                        check(vl->money,vm->money) &&
                                        check(vm->money,mj)){
                                        chunkAns[chunkID].emplace_back(path);
                                    }
                                    for(;vm!=G[vl->to].end();++vm){//i,j,k,l

                                        if( neighbour[vm->to] && !visited[vm->to] && !G[vm->to].empty() && check(vl->money,vm->money)){
                                            step3dfs(mj,vm->money,visited,neighbour,chunkID,chunkAns[chunkID],headNode,vm->to,5,path);
                                        }
                                    }
                                    path.pop_back();
                                    visited[vl->to]=false;
                                }
                            }
                            path.pop_back();
                            visited[vk->to]=false;
                        }
                    }
                    path.pop_back();
                    visited[vj->to]=false;
                }
            }
            path.pop_back();
            visited[i]=false;
        }
    }
#ifdef  TEST
auto end   = system_clock::now();
auto duration = duration_cast<microseconds>(end - start);
PRINT << "Solve "<<chunkID <<" time :" << float(duration.count()) * microseconds::period::num / microseconds::period::den<< "s Chunk loop: "<< chunkAns[chunkID].size()<<endl;
start=system_clock::now();
#endif

}



void oneThreadSolve(void (*func)(uint,uint,uint)){
    // 开启单线程
#ifdef TEST
    PRINT<<"Multithread : "<<endl;
    auto start = system_clock::now();
#endif
    thread t0((*func),0,0,(uint)(nodeCnt*1.0));
#ifdef  TEST
auto end   = system_clock::now();
auto duration = duration_cast<microseconds>(end - start);
    PRINT << "Thread init time :" << float(duration.count()) * microseconds::period::num / microseconds::period::den<< "s" << endl;
#endif
    t0.join();
}

void eightThreadSolve(void (*func)(uint,uint,uint)){
    // 开启8线程，将所有节点分割为八个chunk
    // chunk0:0.00-0.03
    // chunk1:0.03-0.06
    // chunk2:0.06-0.09
    // chunk3:0.09-0.12
    // chunk4:0.12-0.16
    // chunk5:0.16-0.21
    // chunk6:0.21-0.30
    // chunk7:0.30-1.00
#ifdef TEST
    PRINT<<"Multithread : "<<endl;
    auto start = system_clock::now();
#endif
    thread t0((*func),0,0,(uint)(nodeCnt*0.03));
    thread t1((*func),1,(uint)(nodeCnt*0.03),(uint)(nodeCnt*0.06));
    thread t2((*func),2,(uint)(nodeCnt*0.06),(uint)(nodeCnt*0.09));
    thread t3((*func),3,(uint)(nodeCnt*0.09),(uint)(nodeCnt*0.12));
    thread t4((*func),4,(uint)(nodeCnt*0.12),(uint)(nodeCnt*0.16));
    thread t5((*func),5,(uint)(nodeCnt*0.16),(uint)(nodeCnt*0.21));
    thread t6((*func),6,(uint)(nodeCnt*0.21),(uint)(nodeCnt*0.30));
    thread t7((*func),7,(uint)(nodeCnt*0.30),(uint)(nodeCnt));
#ifdef  TEST
auto end   = system_clock::now();
auto duration = duration_cast<microseconds>(end - start);
    PRINT << "Thread init time :" << float(duration.count()) * microseconds::period::num / microseconds::period::den<< "s" << endl;
#endif
    t0.join();
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    t6.join();
    t7.join();
}


void sixteenThreadSolve2(void (*func)(uint,uint,uint)){

#ifdef TEST
    PRINT<<"Multithread : "<<endl;
    auto start = system_clock::now();
#endif
    thread t0((*func),0,0,(uint)(nodeCnt*0.011));
    thread t1((*func),1,(uint)(nodeCnt*0.011),(uint)(nodeCnt*0.022));
    thread t2((*func),2,(uint)(nodeCnt*0.022),(uint)(nodeCnt*0.034));
    thread t3((*func),3,(uint)(nodeCnt*0.034),(uint)(nodeCnt*0.046));
    thread t4((*func),4,(uint)(nodeCnt*0.046),(uint)(nodeCnt*0.059));
    thread t5((*func),5,(uint)(nodeCnt*0.059),(uint)(nodeCnt*0.073));
    thread t6((*func),6,(uint)(nodeCnt*0.073),(uint)(nodeCnt*0.093));
    thread t7((*func),7,(uint)(nodeCnt*0.093),(uint)(nodeCnt*0.112));
    thread t8((*func),8,(uint)(nodeCnt*0.112),(uint)(nodeCnt*0.133));
    thread t9((*func),9,(uint)(nodeCnt*0.133),(uint)(nodeCnt*0.153));
    thread t10((*func),10,(uint)(nodeCnt*0.153),(uint)(nodeCnt*0.183));
    thread t11((*func),11,(uint)(nodeCnt*0.183),(uint)(nodeCnt*0.223));
    thread t12((*func),12,(uint)(nodeCnt*0.223),(uint)(nodeCnt*0.270));
    thread t13((*func),13,(uint)(nodeCnt*0.270),(uint)(nodeCnt*0.330));
    thread t14((*func),14,(uint)(nodeCnt*0.330),(uint)(nodeCnt*0.410));
    thread t15((*func),15,(uint)(nodeCnt*0.410),(uint)(nodeCnt));
#ifdef  TEST
auto end   = system_clock::now();
auto duration = duration_cast<microseconds>(end - start);
    PRINT << "Thread init time :" << float(duration.count()) * microseconds::period::num / microseconds::period::den << "s" << endl;
#endif
    t0.join();
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    t6.join();
    t7.join();
    t8.join();
    t9.join();
    t10.join();
    t11.join();
    t12.join();
    t13.join();
    t14.join();
    t15.join();

}


void fourThreadSolve(void (*func)(uint,uint,uint)){

#ifdef TEST
    PRINT<<"Multithread : "<<endl;
    auto start = system_clock::now();
#endif
    thread t0((*func),0,0,(uint)(nodeCnt*0.046));
    // thread t1((*func),1,(uint)(nodeCnt*0.011),(uint)(nodeCnt*0.022));
    // thread t2((*func),2,(uint)(nodeCnt*0.022),(uint)(nodeCnt*0.034));
    // thread t3((*func),3,(uint)(nodeCnt*0.034),(uint)(nodeCnt*0.046));
    thread t4((*func),1,(uint)(nodeCnt*0.046),(uint)(nodeCnt*0.112));
    // thread t5((*func),5,(uint)(nodeCnt*0.059),(uint)(nodeCnt*0.073));
    // thread t6((*func),6,(uint)(nodeCnt*0.073),(uint)(nodeCnt*0.093));
    // thread t7((*func),7,(uint)(nodeCnt*0.093),(uint)(nodeCnt*0.112));
    thread t8((*func),2,(uint)(nodeCnt*0.112),(uint)(nodeCnt*0.226));
    // thread t9((*func),9,(uint)(nodeCnt*0.133),(uint)(nodeCnt*0.153));
    // thread t10((*func),10,(uint)(nodeCnt*0.153),(uint)(nodeCnt*0.183));
    // thread t11((*func),11,(uint)(nodeCnt*0.183),(uint)(nodeCnt*0.223));
    thread t12((*func),3,(uint)(nodeCnt*0.226),(uint)(nodeCnt));
    // thread t13((*func),13,(uint)(nodeCnt*0.270),(uint)(nodeCnt*0.330));
    // thread t14((*func),14,(uint)(nodeCnt*0.330),(uint)(nodeCnt*0.410));
    // thread t15((*func),15,(uint)(nodeCnt*0.410),(uint)(nodeCnt));
#ifdef  TEST
auto end   = system_clock::now();
auto duration = duration_cast<microseconds>(end - start);
    PRINT << "Thread init time :" << float(duration.count()) * microseconds::period::num / microseconds::period::den << "s" << endl;
#endif
    t0.join();
    t4.join();
    t8.join();
    t12.join();
    // t4.join();
    // t5.join();
    // t6.join();
    // t7.join();
    // t8.join();
    // t9.join();
    // t10.join();
    // t11.join();
    // t12.join();
    // t13.join();
    // t14.join();
    // t15.join();
}



inline void unsigned_to_decimal( uint &len,uint number, char* &buffer )
{
    char* p_first = buffer;
    if( number == 0 )
    {
        *buffer++ = '0';
        len=buffer-p_first;
    }
    else
    {
        while( number != 0 )
        {
            *buffer++ = '0' + number % 10;
            number /= 10;
        }
        reverse( p_first, buffer );
    }
    *buffer++ = ',';
    len=buffer-p_first;
}

inline void writetochar(uint &tLen,char* &test ,Path &path)
{
    // tLen=0;
    uint len=0;
    char buffer[15];
    for(auto x:path){
        unsigned_to_decimal(len,x,test);
        tLen+=len;
    }
    *(test-1)='\n';
    return ;
}

void saveFwrite(string &outputFile){
#ifdef TEST
    PRINT<<"Fwrite save : "<<endl;
    auto start = system_clock::now();
#endif
    uint totalSum=0;

    for(auto &chunkans:chunkAns){
        for(auto &tmpPath:chunkans){
            uint sz = tmpPath.size();
            totalSum++;
            for(auto &x:tmpPath){
                ansString[sz] += idsComma[x];
            }
            ansString[sz][ansString[sz].length()-1]='\n';
        }
    }
#ifdef TEST
    #ifdef  TEST
auto middle   = system_clock::now();
auto duration0 = duration_cast<microseconds>(middle - start);
PRINT << "Catch time :" << float(duration0.count()) * microseconds::period::num / microseconds::period::den << "s" << endl;
#endif

    PRINT<<"Total loop: "<<totalSum<<endl;
#endif
    string totalSum_string = to_string(totalSum)+'\n';
    FILE *fp = fopen(outputFile.c_str(), "wb");
    fwrite(totalSum_string.c_str(),totalSum_string.length(),sizeof(char), fp);
    for(uint i=3;i<8;++i){
        fwrite(ansString[i].c_str(),ansString[i].length(),sizeof(char), fp);
    }
    fclose(fp);
#ifdef  TEST
auto end   = system_clock::now();
auto duration = duration_cast<microseconds>(end - start);
PRINT << "Fwrite time :" << float(duration.count()) * microseconds::period::num / microseconds::period::den << "s" << endl;
#endif
}

void saveWrite(string &outputFile){
#ifdef TEST
    PRINT<<"Mmap save : "<<endl;
    auto start = system_clock::now();
#endif
    uint totalSum=0;

    for(auto &chunkans:chunkAns){
        for(auto &tmpPath:chunkans){
            uint sz = tmpPath.size();
            totalSum++;
            for(auto &x:tmpPath){
                ansString[sz] += idsComma[x];
            }
            ansString[sz][ansString[sz].length()-1]='\n';
        }
    }
#ifdef TEST
    #ifdef  TEST
auto middle   = system_clock::now();
auto duration0 = duration_cast<microseconds>(middle - start);
PRINT << "Catch time :" << float(duration0.count()) * microseconds::period::num / microseconds::period::den << "s" << endl;
#endif

    PRINT<<"Total loop: "<<totalSum<<endl;
#endif
    string totalSum_string = to_string(totalSum)+'\n';
    int fd;
    char *addr;
    fd = open(outputFile.c_str(), O_RDWR | O_CREAT, 0666);   
    int totallen=0;
    totallen += totalSum_string.length();
    for(int i=3;i<8;++i){
        totallen += ansString[i].length();
    }
    // lseek(fd,totallen,SEEK_SET);
    // write(fd,"\n",1);
    // addr = (char *)mmap(NULL, totallen-1, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    write(fd,totalSum_string.c_str() ,totalSum_string.length());
    // memcpy(addr,totalSum_string.c_str() ,totalSum_string.length());
    int tmplen=totalSum_string.length();
    for(int i=3;i<8;++i){
        // memcpy(addr+tmplen,ansString[i].c_str(),ansString[i].length());
        write(fd,ansString[i].c_str(),ansString[i].length());
        tmplen += ansString[i].length();
    }
    // memcpy(addr+tmplen,ansString[7].c_str(),ansString[7].length()-1);
    close(fd);
    // munmap(addr, totallen-1);
    
#ifdef  TEST
auto end   = system_clock::now();
auto duration = duration_cast<microseconds>(end - start);
PRINT << "Mmap time :" << float(duration.count()) * microseconds::period::num / microseconds::period::den << "s" << endl;
#endif
}

void saveMmap(string &outputFile){
#ifdef TEST
    PRINT<<"Mmap save : "<<endl;
    auto start = system_clock::now();
#endif
    uint totalSum=0;

    for(auto &chunkans:chunkAns){
        for(auto &tmpPath:chunkans){
            uint sz = tmpPath.size();
            totalSum++;
            for(auto &x:tmpPath){
                ansString[sz] += idsComma[x];
            }
            ansString[sz][ansString[sz].length()-1]='\n';
        }
    }
#ifdef TEST
    #ifdef  TEST
auto middle   = system_clock::now();
auto duration0 = duration_cast<microseconds>(middle - start);
PRINT << "Catch time :" << float(duration0.count()) * microseconds::period::num / microseconds::period::den << "s" << endl;
#endif

    PRINT<<"Total loop: "<<totalSum<<endl;
#endif
    string totalSum_string = to_string(totalSum)+'\n';
    int fd;
    char *addr;
    fd = open(outputFile.c_str(), O_RDWR | O_CREAT, 0666);   
    int totallen=0;
    totallen += totalSum_string.length();
    for(int i=3;i<8;++i){
        totallen += ansString[i].length();
    }
    lseek(fd,totallen-1,SEEK_SET);
    write(fd,"\n",1);
    addr = (char *)mmap(NULL, totallen-1, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    memcpy(addr,totalSum_string.c_str() ,totalSum_string.length());
    int tmplen=totalSum_string.length();
    for(int i=3;i<7;++i){
        memcpy(addr+tmplen,ansString[i].c_str(),ansString[i].length());
        tmplen += ansString[i].length();
    }
    memcpy(addr+tmplen,ansString[7].c_str(),ansString[7].length()-1);
    close(fd);
    munmap(addr, totallen-1);
    
#ifdef  TEST
auto end   = system_clock::now();
auto duration = duration_cast<microseconds>(end - start);
PRINT << "Mmap time :" << float(duration.count()) * microseconds::period::num / microseconds::period::den << "s" << endl;
#endif
}

int main(int argc, char **argv)
{
    string testFile = "/data/test_data.txt";
    string answerFile = "/projects/student/result.txt";//用户答案
#ifdef TEST
    if(argc>1){
        testFile = (string)("./data/")+(string)(argv[1])+(string)("/test_data.txt");
        answerFile = (string)("./data/")+(string)(argv[1])+(string)("/answer.txt");
    }
    else{
        testFile = "./data/56/test_data.txt";//默认跑官方demo
        answerFile = "./data/56/answer.txt";//用户答案
    }
    auto start=system_clock::now();
#endif
    mmapRead(testFile);
#ifdef TEST
auto end   = system_clock::now();
auto duration = duration_cast<microseconds>(end - start);
    PRINT << "Load data:" << float(duration.count()) * microseconds::period::num / microseconds::period::den << "s" << endl;
    start=system_clock::now();
#endif
    initGraph(true);
#ifdef TEST
end  = system_clock::now();
duration= duration_cast<microseconds>(end - start);
    PRINT << "Init Graph:" << float(duration.count()) * microseconds::period::num / microseconds::period::den << "s" << endl;
    start=system_clock::now();
#endif
    if(lineCnt<10000){
#ifdef TEST
end  = system_clock::now();
duration= duration_cast<microseconds>(end - start);
    PRINT << "8 thread:" << float(duration.count()) * microseconds::period::num / microseconds::period::den << "s" << endl;
    start=system_clock::now();
#endif
        oneThreadSolve(step3cirChunk);
    }else{
        sixteenThreadSolve2(step3cirChunk);
    }
#ifdef TEST
end  = system_clock::now();
duration= duration_cast<microseconds>(end - start);
    PRINT << "Multithread :" << float(duration.count()) * microseconds::period::num / microseconds::period::den << "s" << endl;
    start=system_clock::now();
#endif
    // saveFwrite(answerFile);
    // saveMmap(answerFile);
    saveWrite(answerFile);

    return 0;
}

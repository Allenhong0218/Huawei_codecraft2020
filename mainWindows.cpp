// #define LINUX
#define WINDOWS


#ifdef LINUX
    #include <bits/stdc++.h>
    #include <sys/mman.h>
    #include <fcntl.h>
    #include <sys/stat.h>
    #include <thread>
    #include <atomic>
    #include <unistd.h>
    #include <chrono>   
#endif

#ifdef WINDOWS
    #include <vector>
    #include <thread>
    #include <atomic>
    #include <chrono>
    #include <iostream>
    #include <algorithm>
    #include <unordered_map>
#endif

using namespace std;
using namespace std::chrono;

#define TEST
#define PRINT cout
#define MAXTHREADNUM 4

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
vector<uint> chunkAnsSize[MAXTHREADNUM];

string ansString[8];
uint nodeCnt;
uint ansCnt;
uint lineCnt;
char* buf;//mmapped buffer

#ifdef LINUX
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
#endif

void fscanfRead(string &testFile) {
    FILE *file = fopen(testFile.c_str(), "r");
    uint u, v, c;
    // int cnt = 0;
    while (fscanf(file, "%u,%u,%u\\r", &u, &v, &c) != EOF) {
        inputs.push_back(u);
        inputs.push_back(v);
        // migic[(ull) u << 32 | v] = c;
        moneys.push_back(c);
        // PRINT<<u<<"->"<<v<<": "<<c<<endl;
        ++lineCnt;
    }
#ifdef TEST
    printf("%d Records in Total\n", lineCnt);
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


inline void get3neighbour(Node &vi,vector<bool> &neighbour1,vector<bool> &neighbour2,vector<bool> &neighbour3){
    auto &Gi=Ginv[vi.to];
    if(!Gi.empty()){
        auto vj =lower_bound(Gi.begin(),Gi.end(),vi);
        for(;vj!=Gi.end();++vj){
            auto mj = vj->money;
            neighbour1[vj->to]=true;
            neighbour2[vj->to]=true;
            neighbour3[vj->to]=true;
            auto &Gj = Ginv[vj->to];
            if(!Gj.empty()){
                auto vk = lower_bound(Gj.begin(),Gj.end(),vi);                
                for(;vk!=Gj.end();++vk){
                        auto mk = vk->money;
                        if(check(mk,mj)){
                            neighbour1[vk->to]=true;
                            neighbour2[vk->to]=true;
                            auto &Gk = Ginv[vk->to];
                            if(!Gk.empty()){
                                auto vl = lower_bound(Gk.begin(),Gk.end(),vi);
                                for(;vl!=Gk.end();++vl){
                                    auto ml = vl->money;
                                    if(check(ml,mk)){
                                        neighbour1[vl->to]=true;
                                    }
                                        
                                }
                            }
                        }
                }
            }

        }
    }

}

void balancedCirChunk(uint chunkID,uint idxBegin,uint idxEnd){
    //寻找chunk中的环
#ifdef TEST
    auto start = system_clock::now();
#endif
    for(uint i=idxBegin;i<idxEnd;i+=4){
        uint tmpAnsSize = 0;
        if(!G[i].empty()){//i->j->k->l->m->n->o->i
            vector<bool>visited(nodeCnt,false);
            vector<uint> path;
            path.push_back(i);//head
            visited[i]=true;
            vector<bool>neighbour1(nodeCnt,false);
            vector<bool>neighbour2(nodeCnt,false);
            vector<bool>neighbour3(nodeCnt,false);
            auto headNode = Node(i,0);
            get3neighbour(headNode,neighbour1,neighbour2,neighbour3);
            auto vj = lower_bound(G[i].begin(),G[i].end(),headNode);
            for(;vj!=G[i].end();++vj){
                if(!visited[vj->to] && !G[vj->to].empty()){
                    path.push_back(vj->to);//i,j
                    visited[vj->to]=true;
                    auto mj = vj->money;
                    auto vk = lower_bound(G[vj->to].begin(),G[vj->to].end(),headNode);
                    for(;vk!=G[vj->to].end();++vk){
                        auto mk = vk->money;
                        if(!visited[vk->to] && check(mj,mk)&& !G[vk->to].empty()){
                            path.push_back(vk->to);//i,j,k
                            visited[vk->to]=true;
                            auto vl = lower_bound(G[vk->to].begin(),G[vk->to].end(),headNode);
                            
                            if( neighbour3[vk->to] && 
                                check(mk,vl->money) &&
                                check(vl->money,mj)){//find cylcle,depth=3
                                ++tmpAnsSize;
                                chunkAns[chunkID].emplace_back(path);
                            }
                            for(;vl!=G[vk->to].end();++vl){//
                                if(!visited[vl->to] && check(mk,vl->money)&& !G[vl->to].empty() ){
                                    path.push_back(vl->to);//i,j,k,l,path长为4
                                    visited[vl->to]=true;
                                    auto vm = lower_bound(G[vl->to].begin(),G[vl->to].end(),headNode);
                                    if( neighbour3[vl->to]&& 
                                        check(vl->money,vm->money) &&
                                        check(vm->money,mj)){
                                            ++tmpAnsSize;
                                        chunkAns[chunkID].emplace_back(path);
                                    }
                                    for(;vm!=G[vl->to].end();++vm){//
                                        if( neighbour1[vm->to] && !visited[vm->to] && check(vl->money,vm->money)&& !G[vm->to].empty()){
                                            path.push_back(vm->to);//i,j,k,l,m,path长度为5
                                            visited[vm->to] = true;
                                            auto vn = lower_bound(G[vm->to].begin(),G[vm->to].end(),headNode);
                                            if( neighbour3[vm->to]&& 
                                                check(vm->money,vn->money) &&
                                                check(vn->money,mj)){
                                                    ++tmpAnsSize;
                                                chunkAns[chunkID].emplace_back(path);
                                            }
                                            for(;vn!=G[vm->to].end();++vn){//
                                                if(neighbour2[vn->to] && !visited[vn->to] && check(vm->money,vn->money)&& !G[vn->to].empty()){
                                                    path.push_back(vn->to);//i,j,k,l,m,n,path长度为6
                                                    visited[vn->to] = true;
                                                    auto vo =  lower_bound(G[vn->to].begin(),G[vn->to].end(),headNode);
                                                    if( neighbour3[vn->to] && 
                                                        check(vn->money,vo->money) &&
                                                        check(vo->money,mj)){
                                                        ++tmpAnsSize;
                                                        chunkAns[chunkID].emplace_back(path);
                                                    }
                                                    for(;vo!=G[vn->to].end();++vo){
                                                        if(neighbour3[vo->to] && !visited[vo->to]  && check(vn->money,vo->money)&& !G[vo->to].empty()){
                                                            path.push_back(vo->to);//i,j,k,l,m,n,o,path长度为7
                                                            auto vp = lower_bound(G[vo->to].begin(),G[vo->to].end(),headNode);
                                                            if( check(vo->money,vp->money) &&
                                                                check(vp->money,mj)){
                                                                ++tmpAnsSize;
                                                                chunkAns[chunkID].emplace_back(path);
                                                            }
                                                            path.pop_back();
                                                        }
                                                    }
                                                    path.pop_back();
                                                    visited[vn->to] = false;

                                                }
                                            }
                                            path.pop_back();
                                            visited[vm->to] = false;
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
        chunkAnsSize[chunkID].push_back(tmpAnsSize);
    }
#ifdef  TEST
auto end   = system_clock::now();
auto duration = duration_cast<microseconds>(end - start);
PRINT << "Solve "<<chunkID <<" time :" << float(duration.count()) * microseconds::period::num / microseconds::period::den<< "s Chunk loop: "<< chunkAns[chunkID].size()<<endl;
start=system_clock::now();
#endif

}


void balancedFourThread(void (*func)(uint,uint,uint)){

#ifdef TEST
    PRINT<<"Multithread : "<<endl;
    auto start = system_clock::now();
#endif
    thread t0((*func),0,0,nodeCnt);
    thread t1((*func),1,1,nodeCnt);
    thread t2((*func),2,2,nodeCnt);
    thread t3((*func),3,3,nodeCnt);
#ifdef  TEST
auto end   = system_clock::now();
auto duration = duration_cast<microseconds>(end - start);
    PRINT << "Thread init time :" << float(duration.count()) * microseconds::period::num / microseconds::period::den << "s" << endl;
#endif
    t0.join();
    t1.join();
    t2.join();
    t3.join();
}

void saveFwrite(string &outputFile){
#ifdef TEST
    PRINT<<"Fwrite save : "<<endl;
    auto start = steady_clock::now();
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
auto middle   = steady_clock::now();
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
auto end   = steady_clock::now();
auto duration = duration_cast<microseconds>(end - start);
PRINT << "Fwrite time :" << float(duration.count()) * microseconds::period::num / microseconds::period::den << "s" << endl;
#endif
}

#ifdef LINUX
void saveWrite(string &outputFile){
#ifdef TEST
    PRINT<<"Mmap save : "<<endl;
    auto start = steady_clock::now();
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
auto middle   = steady_clock::now();
auto duration0 = duration_cast<microseconds>(middle - start);
PRINT << "Catch time :" << float(duration0.count()) * microseconds::period::num / microseconds::period::den << "s" << endl;
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
    write(fd,totalSum_string.c_str() ,totalSum_string.length());
    int tmplen=totalSum_string.length();
    for(int i=3;i<8;++i){
        write(fd,ansString[i].c_str(),ansString[i].length());
        tmplen += ansString[i].length();
    }
    close(fd);

#ifdef  TEST
auto end   = steady_clock::now();
auto duration = duration_cast<microseconds>(end - start);
PRINT << "Write time :" << float(duration.count()) * microseconds::period::num / microseconds::period::den << "s" << endl;
#endif
}

void saveMmap(string &outputFile){
#ifdef TEST
    PRINT<<"Mmap save : "<<endl;
    auto start = steady_clock::now();
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
auto middle   = steady_clock::now();
auto duration0 = duration_cast<microseconds>(middle - start);
PRINT << "Catch time :" << float(duration0.count()) * microseconds::period::num / microseconds::period::den << "s" << endl;
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
auto end   = steady_clock::now();
auto duration = duration_cast<microseconds>(end - start);
PRINT << "Mmap time :" << float(duration.count()) * microseconds::period::num / microseconds::period::den << "s" << endl;
#endif
}
#endif

int main(int argc, char **argv)
{
#ifdef LINUX
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
        auto start=steady_clock::now();
    #endif

#endif

#ifdef WINDOWS
    string testFile = "./test_data.txt";
    string answerFile = "./result.txt";
    auto start=steady_clock::now();

#endif

#ifdef LINUX 
    mmapRead(testFile);
#endif
#ifdef WINDOWS
    fscanfRead(testFile);
#endif
#ifdef TEST
    auto end   = steady_clock::now();
    auto duration = duration_cast<microseconds>(end - start);
    PRINT << "Load data:" << float(duration.count()) * microseconds::period::num / microseconds::period::den << "s" << endl;
    start=steady_clock::now();
#endif
    initGraph(true);
#ifdef TEST
    end  = steady_clock::now();
    duration= duration_cast<microseconds>(end - start);
    PRINT << "Init Graph:" << float(duration.count()) * microseconds::period::num / microseconds::period::den << "s" << endl;
    start=steady_clock::now();
#endif
    if(lineCnt<10000){
#ifdef TEST
    end  = steady_clock::now();
    duration= duration_cast<microseconds>(end - start);
    PRINT << "8 thread:" << float(duration.count()) * microseconds::period::num / microseconds::period::den << "s" << endl;
    start=steady_clock::now();
#endif
        // oneThreadSolve(step3cirChunk);
    }else{
        balancedFourThread(balancedCirChunk);
    }
#ifdef TEST
    end  = steady_clock::now();
    duration= duration_cast<microseconds>(end - start);
    PRINT << "Multithread :" << float(duration.count()) * microseconds::period::num / microseconds::period::den << "s" << endl;
    start=steady_clock::now();
#endif
#ifdef WINDOWS
    saveFwrite(answerFile);
#endif
#ifdef LINUX
    // saveMmap(answerFile);
    saveWrite(answerFile);
#endif

    return 0;
}

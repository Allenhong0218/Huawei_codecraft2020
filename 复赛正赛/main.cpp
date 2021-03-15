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
#define MAXTHREADNUM 4

#define MAXCHARSIZE7 4500000000
#define MAXCHARSIZE  900000000

char ansChar8[MAXCHARSIZE7];
char ansChar7[MAXCHARSIZE7];
char ansChar6[MAXCHARSIZE];
char ansChar5[MAXCHARSIZE];
char ansChar4[MAXCHARSIZE];
char ansChar3[MAXCHARSIZE];
typedef long long ull;
typedef vector<ull> Path;


struct Node{
    ull to;
    ull money;
    Node(ull x,ull y){to=x;money=y;}
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
vector<char[20]> charidsComma;
unordered_map<ull,ull> idHash; 
vector<ull> inputs;
vector<ull> moneys;

vector<Path> chunkAns[MAXTHREADNUM];//最多可开启16线程
vector<ull> chunkAnsSize[MAXTHREADNUM];


string ansString[9];
ull nodeCnt;
ull ansCnt;
ull lineCnt;
char* buf;//mmapped buffer


inline void unsigned_to_decimal(int number, char* buffer )
{
    char* p_first = buffer;
    if( number == 0 )
    {
        *buffer++ = '0';
        // len=buffer-p_first;
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
    *buffer = '\0';
}

void mmapRead(string &testFile){
    ull fd = open(testFile.c_str(), O_RDONLY);
    ull test_data_size = lseek(fd, 0, SEEK_END);
    buf = (char*)mmap(NULL, test_data_size, PROT_READ, MAP_PRIVATE, fd, 0);
    ull test=0;
    lineCnt=0;
    ull tmp=test_data_size;
    // bool find_dot=false;
    bool flag=false;
    ull find_dot=0;
    while(tmp--){
        if(*buf==','){
            inputs.push_back(test);
            test = 0;
            ++buf;
            continue;
        }
        if(*buf=='\n'){//金额数据，初赛并不需要使用
            if(flag){
                ull cnt_xiaoshu = find_dot-tmp-2;
                if( cnt_xiaoshu == 2){
                    moneys.push_back(test);
                }else{
                    moneys.push_back(test*10ll);
                }
            }else{
                moneys.push_back(test*100ll);
            }
            ++lineCnt;
            test=0;
            ++buf;
            flag=false;
            continue;
        }
        if(*buf=='\r'){
            // cnt_xiaoshu=tmp;
            ++buf;
            continue;
        }
        if(*buf=='.'){
            flag=true;
            find_dot=tmp;
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
    nodeCnt=tmp.size();
    charidsComma = vector<char[20]>(nodeCnt);
    auto tmpcnt=0;
    for(auto &x:tmp){
        // idsComma.push_back(to_string(x)+',');
        unsigned_to_decimal(x,charidsComma[tmpcnt]);
        // if(tmpcnt==100){
        //     PRINT<<x<<": :"<<charidsComma[tmpcnt]<<endl;
        // }
        idHash[x]=tmpcnt++;
    }
#ifdef TEST
    PRINT<< nodeCnt <<"Nodes in Total"<<endl;
#endif
    ull sz=inputs.size();
    G=vector<vector<Node>>(nodeCnt);
    ull *inDegrees=new ull[nodeCnt];
    ull *outDegrees=new ull[nodeCnt];
    if(!initGinv){
        for(ull i=0;i<sz;i+=2){
            ull u=idHash[inputs[i]],v=idHash[inputs[i+1]];
            auto money = moneys[i/2];
            G[u].push_back(Node(v,money));
            ++inDegrees[v];
            ++outDegrees[u];
        }
    }
    else{
        Ginv=vector<vector<Node>>(nodeCnt);
        for(ull i=0;i<sz;i+=2){
            ull u=idHash[inputs[i]],v=idHash[inputs[i+1]];
            auto money = moneys[i/2];
            // PRINT<<"Money:"<<money<<endl;
            G[u].push_back(Node(v,money));
            Ginv[v].push_back(Node(u,money));
            ++inDegrees[v];
            ++outDegrees[u];
        }      
    }
    vector<ull>inRemove,outRemove;
    for(ull i=0;i<nodeCnt;i++){
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
    for(ull i=0;i<nodeCnt;i++){
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

inline bool check(ull x,ull y){
    return x<=5ll*y && y<=3ll*x;
}

inline void get3neighbour(Node &vi,vector<bool> &neighbour1,vector<bool> &neighbour2,vector<bool> &neighbour3, vector<bool> &neighbour4){
    auto &Gi=Ginv[vi.to];
    if(!Gi.empty()){
        auto vj =lower_bound(Gi.begin(),Gi.end(),vi);
        for(;vj!=Gi.end();++vj){
            auto mj = vj->money;
            neighbour1[vj->to]=true;
            neighbour2[vj->to]=true;
            neighbour3[vj->to]=true;
            neighbour4[vj->to]=true;
            auto &Gj = Ginv[vj->to];
            if(!Gj.empty()){
                auto vk = lower_bound(Gj.begin(),Gj.end(),vi);                
                for(;vk!=Gj.end();++vk){
                        auto mk = vk->money;
                        if(check(mk,mj)){
                            neighbour1[vk->to]=true;
                            neighbour2[vk->to]=true;
                            neighbour3[vk->to]=true;
                            auto &Gk = Ginv[vk->to];
                            if(!Gk.empty()){
                                auto vl = lower_bound(Gk.begin(),Gk.end(),vi);
                                for(;vl!=Gk.end();++vl){
                                    auto ml = vl->money;
                                    if(check(ml,mk)){
                                        neighbour1[vl->to]=true;
                                        neighbour2[vl->to]=true;
                                        auto &Gl = Ginv[vl->to];
                                        if(!Gl.empty()){
                                            auto vm = lower_bound(Gl.begin(), Gl.end(), vi);
                                            for(;vm!=Gl.end();++vm){
                                                auto mm = vm->money;
                                                if(check(mm, ml)){
                                                    neighbour1[vm->to]=true;
                                                }
                                            }
                                        }
                                    }
                                        
                                }
                            }
                        }
                }
            }

        }
    }

}

void balancedCirChunk(ull chunkID,ull idxBegin,ull idxEnd){
    //寻找chunk中的环
#ifdef TEST
    auto start = system_clock::now();
#endif
    for(ull i=idxBegin;i<idxEnd;i+=4){
        ull tmpAnsSize = 0;
        if(!G[i].empty()){//i->j->k->l->m->n->o->i
            
            vector<bool>visited(nodeCnt,false);
            vector<ull> path(8);
            // path[0]=i;//head
            auto tmp_path_it = path.begin();
            *tmp_path_it++ = i;
            visited[i]=true;
            vector<bool>neighbour1(nodeCnt,false);
            vector<bool>neighbour2(nodeCnt,false);
            vector<bool>neighbour3(nodeCnt,false);
            vector<bool>neighbour4(nodeCnt,false);
            auto headNode = Node(i,0);
            get3neighbour(headNode,neighbour1,neighbour2,neighbour3,neighbour4);
            auto vj = lower_bound(G[i].begin(),G[i].end(),headNode);
            for(;vj!=G[i].end();++vj){
                if(!visited[vj->to] && !G[vj->to].empty()){
                    *tmp_path_it++=vj->to;//i,j
                    visited[vj->to]=true;
                    auto mj = vj->money;
                    auto vk = lower_bound(G[vj->to].begin(),G[vj->to].end(),headNode);
                    for(;vk!=G[vj->to].end();++vk){
                        auto mk = vk->money;
                        if(!visited[vk->to] && check(mj,mk)&& !G[vk->to].empty()){
                            *tmp_path_it++ = vk->to;//i,j,k
                            visited[vk->to]=true;
                            auto vl = lower_bound(G[vk->to].begin(),G[vk->to].end(),headNode);
                            
                            if( neighbour4[vk->to] && 
                                check(mk,vl->money) &&
                                check(vl->money,mj)){//find cylcle,depth=3
                                ++tmpAnsSize;
                                chunkAns[chunkID].emplace_back(Path(path.begin(),tmp_path_it));
                            }
                            for(;vl!=G[vk->to].end();++vl){//
                                if(!visited[vl->to] && check(mk,vl->money)&& !G[vl->to].empty() ){
                                    *tmp_path_it++ = vl->to;//i,j,k,l,path长为4
                                    visited[vl->to]=true;
                                    auto vm = lower_bound(G[vl->to].begin(),G[vl->to].end(),headNode);
                                    if( neighbour4[vl->to]&& 
                                        check(vl->money,vm->money) &&
                                        check(vm->money,mj)){
                                            ++tmpAnsSize;
                                        chunkAns[chunkID].emplace_back(Path(path.begin(),tmp_path_it));
                                    }
                                    for(;vm!=G[vl->to].end();++vm){//
                                        if( neighbour1[vm->to] && !visited[vm->to] && check(vl->money,vm->money)&& !G[vm->to].empty()){
                                            *tmp_path_it++ = vm->to;//i,j,k,l,m,path长度为5
                                            visited[vm->to] = true;
                                            auto vn = lower_bound(G[vm->to].begin(),G[vm->to].end(),headNode);
                                            if( neighbour4[vm->to]&& 
                                                check(vm->money,vn->money) &&
                                                check(vn->money,mj)){
                                                    ++tmpAnsSize;
                                                chunkAns[chunkID].emplace_back(Path(path.begin(),tmp_path_it));
                                            }
                                            for(;vn!=G[vm->to].end();++vn){//
                                                if(neighbour2[vn->to] && !visited[vn->to] && check(vm->money,vn->money)&& !G[vn->to].empty()){
                                                    *tmp_path_it++ = vn->to;//i,j,k,l,m,n,path长度为6
                                                    visited[vn->to] = true;
                                                    auto vo =  lower_bound(G[vn->to].begin(),G[vn->to].end(),headNode);
                                                    if( neighbour4[vn->to] && 
                                                        check(vn->money,vo->money) &&
                                                        check(vo->money,mj)){
                                                        ++tmpAnsSize;
                                                        chunkAns[chunkID].emplace_back(Path(path.begin(),tmp_path_it));
                                                    }
                                                    for(;vo!=G[vn->to].end();++vo){
                                                        if(neighbour3[vo->to] && !visited[vo->to]  && check(vn->money,vo->money)&& !G[vo->to].empty()){
                                                            *tmp_path_it++ = vo->to;//i,j,k,l,m,n,o,path长度为7
                                                            visited[vo->to] = true;
                                                            auto vp = lower_bound(G[vo->to].begin(),G[vo->to].end(),headNode);
                                                            if( neighbour4[vo->to] &&
                                                                check(vo->money,vp->money) &&
                                                                check(vp->money,mj)){
                                                                ++tmpAnsSize;
                                                                chunkAns[chunkID].emplace_back(Path(path.begin(),tmp_path_it));
                                                            }
                                                            for(;vp!=G[vo->to].end();++vp){
                                                                if(neighbour4[vp->to]&&!visited[vp->to] && check(vo->money,vp->money)&& !G[vp->to].empty()){
                                                                    *tmp_path_it = vp->to;//i,j,k,l,m,n,o,p,path长度为8
                                                                    auto vq = lower_bound(G[vp->to].begin(),G[vp->to].end(),headNode);
                                                                    if( check(vp->money,vq->money) &&
                                                                        check(vq->money,mj)){
                                                                        ++tmpAnsSize;
                                                                        chunkAns[chunkID].emplace_back(path);
                                                                    }
                                                                }
                                                            }
                                                            // path.pop_back();

                                                            --tmp_path_it;
                                                            visited[vo->to] = false;
                                                        }
                                                    }
                                                    // path.pop_back();
                                                    --tmp_path_it;
                                                    visited[vn->to] = false;

                                                }
                                            }
                                            // path.pop_back();
                                            --tmp_path_it;
                                            visited[vm->to] = false;
                                            // step3dfs(mj,vm->money,visited,neighbour,chunkID,chunkAns[chunkID],headNode,vm->to,5,path);
                                        }
                                    }
                                    // path.pop_back();
                                    --tmp_path_it;
                                    visited[vl->to]=false;
                                }
                            }
                            // path.pop_back();
                            --tmp_path_it;
                            visited[vk->to]=false;
                        }
                    }
                    // path.pop_back();
                    --tmp_path_it;
                    visited[vj->to]=false;
                }
            }
            // path.pop_back();
            --tmp_path_it;
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

void balancedFourThread(void (*func)(ull,ull,ull)){

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

void balencedSaveWrite(string &outputFile){
#ifdef TEST
    PRINT<<"balanced save : "<<endl;
    auto start = system_clock::now();
#endif
    ull totalSum=0;
    ull base[4];
    for(ull i=0;i<4;++i){
        base[i] = 0;
        totalSum += chunkAns[i].size();
    }
    ull maxCnt = nodeCnt/4;
    for(ull i =0;i<maxCnt;++i){
        for(ull j=0;j<chunkAnsSize[0][i];++j){
            auto tmpPath = chunkAns[0][base[0]+j];
            ull sz = tmpPath.size();
            for(auto &x:tmpPath){
                ansString[sz] += idsComma[x];
            }
            ansString[sz][ansString[sz].length()-1]='\n';
        }
        for(ull j=0;j<chunkAnsSize[1][i];++j){
            auto tmpPath = chunkAns[1][base[1]+j];
            ull sz = tmpPath.size();
            for(auto &x:tmpPath){
                ansString[sz] += idsComma[x];
            }
            ansString[sz][ansString[sz].length()-1]='\n';
        }
        for(ull j=0;j<chunkAnsSize[2][i];++j){
            auto tmpPath = chunkAns[2][base[2]+j];
            ull sz = tmpPath.size();
            for(auto &x:tmpPath){
                ansString[sz] += idsComma[x];
            }
            ansString[sz][ansString[sz].length()-1]='\n';
        }
        for(ull j=0;j<chunkAnsSize[3][i];++j){
            auto tmpPath = chunkAns[3][base[3]+j];
            ull sz = tmpPath.size();
            for(auto &x:tmpPath){
                ansString[sz] += idsComma[x];
            }
            ansString[sz][ansString[sz].length()-1]='\n';
        }
        base[0] += chunkAnsSize[0][i];
        base[1] += chunkAnsSize[1][i];
        base[2] += chunkAnsSize[2][i];
        base[3] += chunkAnsSize[3][i];
    }
    // PRINT<<"4 is ok"<<endl;
    ull resCnt = nodeCnt%4;
    // PRINT<<
    for(ull i=0;i<resCnt;++i){
        for(ull j=0;j<chunkAnsSize[i][maxCnt+i];++j){
            auto tmpPath = chunkAns[i][base[i]+j];
            ull sz = tmpPath.size();
            for(auto &x:tmpPath){
                ansString[sz] += idsComma[x];
            }
            ansString[sz][ansString[sz].length()-1]='\n';
        }
    }
    
#ifdef TEST
    auto middle   = system_clock::now();
    auto duration0 = duration_cast<microseconds>(middle - start);
    PRINT << "Catch time :" << float(duration0.count()) * microseconds::period::num / microseconds::period::den << "s" << endl;
    PRINT<<"Total loop: "<<totalSum<<endl;
#endif
    string totalSum_string = to_string(totalSum)+'\n';
    ull fd;
    char *addr;
    fd = open(outputFile.c_str(), O_RDWR | O_CREAT, 0666);   
    ull totallen=0;
    totallen += totalSum_string.length();
    for(ull i=3;i<9;++i){
        totallen += ansString[i].length();
    }
    write(fd,totalSum_string.c_str() ,totalSum_string.length());
    ull tmplen=totalSum_string.length();
    for(ull i=3;i<9;++i){
        write(fd,ansString[i].c_str(),ansString[i].length());
        tmplen += ansString[i].length();
    }
    close(fd);
    
#ifdef  TEST
    auto end   = system_clock::now();
    auto duration = duration_cast<microseconds>(end - start);
    PRINT << "Write time :" << float(duration.count()) * microseconds::period::num / microseconds::period::den << "s" << endl;
#endif
}



void balencedCharWrite(string &outputFile){
#ifdef TEST
    PRINT<<"balanced char save : "<<endl;
    auto start = system_clock::now();
#endif
    ull totalSum=0;
    ull base[4];
    for(ull i=0;i<4;++i){
        base[i] = 0;
        totalSum += chunkAns[i].size();
    }
    ull maxCnt = nodeCnt/4;
    char *ansChar_head[]={ansChar3,ansChar4,ansChar5,ansChar6,ansChar7,ansChar8};
    char *ansChar_end[]={ansChar3,ansChar4,ansChar5,ansChar6,ansChar7,ansChar8};
    for(ull i =0;i<maxCnt;++i){
        for(ull j=0;j<chunkAnsSize[0][i];++j){
            auto tmpPath = chunkAns[0][base[0]+j];
            ull sz = tmpPath.size()-3;
            for(auto &x:tmpPath){
                // ansString[sz] += idsComma[x];
                // while()
                for(char *y=charidsComma[x];*y!='\0';++y){
                    *ansChar_end[sz]++ = *y;
                }
            }
            // ansString[sz][ansString[sz].length()-1]='\n';
            *(ansChar_end[sz]-1) = '\n';
        }
        for(ull j=0;j<chunkAnsSize[1][i];++j){
            auto tmpPath = chunkAns[1][base[1]+j];
            ull sz = tmpPath.size()-3;
            for(auto &x:tmpPath){
                // ansString[sz] += charidsComma[x];
                // while()
                for(char *y=charidsComma[x];*y!='\0';++y){
                    *ansChar_end[sz]++ = *y;
                }
            }
            // ansString[sz][ansString[sz].length()-1]='\n';
            *(ansChar_end[sz]-1) = '\n';
        }
        for(ull j=0;j<chunkAnsSize[2][i];++j){
            auto tmpPath = chunkAns[2][base[2]+j];
            ull sz = tmpPath.size()-3;
            for(auto &x:tmpPath){
                // ansString[sz] += charidsComma[x];
                // while()
                for(char *y=charidsComma[x];*y!='\0';++y){
                    *ansChar_end[sz]++ = *y;
                }
            }
            // ansString[sz][ansString[sz].length()-1]='\n';
            *(ansChar_end[sz]-1) = '\n';
        }
        for(ull j=0;j<chunkAnsSize[3][i];++j){
            auto tmpPath = chunkAns[3][base[3]+j];
            ull sz = tmpPath.size()-3;
            for(auto &x:tmpPath){
                // ansString[sz] += charidsComma[x];
                // while()
                for(char *y=charidsComma[x];*y!='\0';++y){
                    *ansChar_end[sz]++ = *y;
                }
            }
            // ansString[sz][ansString[sz].length()-1]='\n';
            *(ansChar_end[sz]-1) = '\n';
        }
        base[0] += chunkAnsSize[0][i];
        base[1] += chunkAnsSize[1][i];
        base[2] += chunkAnsSize[2][i];
        base[3] += chunkAnsSize[3][i];
    }
    // PRINT<<"4 is ok"<<endl;
    ull resCnt = nodeCnt%4;
    // PRINT<<
    for(ull i=0;i<resCnt;++i){
        for(ull j=0;j<chunkAnsSize[i][maxCnt+i];++j){
            auto tmpPath = chunkAns[i][base[i]+j];
            ull sz = tmpPath.size()-3;
            for(auto &x:tmpPath){
                // ansString[sz] += charidsComma[x];
                // while()
                for(char *y=charidsComma[x];*y!='\0';++y){
                    *ansChar_end[sz]++ = *y;
                }
            }
            // ansString[sz][ansString[sz].length()-1]='\n';
            *(ansChar_end[sz]-1) = '\n';
        }
    }
    
#ifdef TEST
    auto middle   = system_clock::now();
    auto duration0 = duration_cast<microseconds>(middle - start);
    PRINT << "Catch time :" << float(duration0.count()) * microseconds::period::num / microseconds::period::den << "s" << endl;
    PRINT<<"Total loop: "<<totalSum<<endl;
#endif
    string totalSum_string = to_string(totalSum)+'\n';
    ull fd;
    fd = open(outputFile.c_str(), O_RDWR | O_CREAT, 0666);   
    write(fd,totalSum_string.c_str() ,totalSum_string.length());
    for(ull i=0;i<6;++i){
        write(fd,ansChar_head[i],ansChar_end[i]-ansChar_head[i]);
    }
    close(fd);
    
#ifdef  TEST
    auto end   = system_clock::now();
    auto duration = duration_cast<microseconds>(end - start);
    PRINT << "Write time :" << float(duration.count()) * microseconds::period::num / microseconds::period::den << "s" << endl;
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
    balancedFourThread(balancedCirChunk);
#ifdef TEST
    end  = system_clock::now();
    duration= duration_cast<microseconds>(end - start);
    PRINT << "Multithread :" << float(duration.count()) * microseconds::period::num / microseconds::period::den << "s" << endl;
    start=system_clock::now();
#endif
    // balencedSaveWrite(answerFile);
    balencedCharWrite(answerFile);
    return 0;
}

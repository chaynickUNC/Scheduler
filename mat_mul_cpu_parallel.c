// C program to multiply two square matrices. 
#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <thread>
#include <time.h>
#include <ctime>
#include <sys/time.h>
#include <utility>
#include <cstring>
#include <string>
#include <vector>
#include <tuple>
#include <map>

#define N 256 
#define LOG_LEVEL 0
#define DATATYPE float
#define MULTICORE 0

typedef struct _temperature{
    
    std::vector<int> sensors;
    int cpuB_0;
    int cpuB_1;
    int cpuB_2;
    int cpuB_3;
    int gpu_4;

} Temperature;

struct arg_struct {
    DATATYPE* arg1;
    DATATYPE* arg2;
    DATATYPE* arg3;
    int arg4;
};

FILE *tmp_result;
int finish;
unsigned long long int START_TIME;
std::map<unsigned long long int,Temperature*> tempMap;
  
void print_temperature(Temperature* tmp){
    printf("\nTemperature: %d %d %d %d %d",tmp->sensors[0],tmp->sensors[1],tmp->sensors[2],tmp->sensors[3],tmp->sensors[4]); 
}

unsigned long long int get_current_time(){
    struct timeval c_time;
    gettimeofday(&c_time,NULL);
    unsigned long long int current_time=(unsigned long long int )(c_time.tv_sec*1000000+c_time.tv_usec);
    //return (unsigned long long int)(current_time - start_time)*timeFactor;
    return (unsigned long long int)(current_time - START_TIME);
}

int check_core_temperature(int sensor)
{
    int core;
    switch (sensor)
    {
        case 0: core=0;
            break;
        case 1: core=3;
            break;
        case 2: core=2;
            break;
        case 3: core = 1;
            break;
        case 4: core = 4;
            break;         
    }

    char filename[1024];
    strcpy(filename,"/sys/class/thermal/thermal_zone"); 
    std::string s = std::to_string(core);
    char const *ichar = s.c_str();
    strcat(filename,ichar);
    strcat(filename,"/temp");
    FILE *fp;     
    fp = fopen(filename, "r"); // read mode
    if (fp == NULL)
    exit(EXIT_FAILURE); 
    char* tmp;
    char line[100];
    while (fgets(line, 10, fp))    
    {   
        // tmp = strdup(line); 
        tmp = (char*)calloc(strlen(line) + 1,1 );
        strcpy(tmp, line);
        // printf("monitor_temperature: tmp: %s\n",tmp);
    }
    fclose(fp);
    return atoi(tmp);
}

void monitor_temperature()
{    
    Temperature * t=new Temperature();
    for(int i=0;i<5;i++)
    {      
        int tmp=check_core_temperature(i);  
        // std::vector<int> sensors;
        // t->sensors=sensors;
        // t->sensors.push_back(atoi(tmp));
        // printf("tmp %d\n",atoi(tmp));
        if(i==0){
            t->cpuB_0=tmp;
            t->sensors.push_back(t->cpuB_0);
        }
        else if(i==1){
            t->cpuB_1=tmp;
            t->sensors.push_back(t->cpuB_1);
        }
        else if(i==2){
            t->cpuB_2=tmp;
            t->sensors.push_back(t->cpuB_2);
        }
        else if(i==3){
            t->cpuB_3=tmp;
            t->sensors.push_back(t->cpuB_3);
        }
        else if(i==4){
            t->gpu_4=tmp; 
            t->sensors.push_back(t->gpu_4);            
        }
    }
    tempMap.insert( std::pair<unsigned long long int, Temperature*>(get_current_time(), t) );  
    //print_temperature(t)   ;  
}

void *temperature_monitor(void *vargp){
    cpu_set_t cpuset;
    pthread_t thread_id_moniter = pthread_self(); 
    CPU_ZERO(&cpuset);
    CPU_SET(2, &cpuset);    
    int s = pthread_setaffinity_np(thread_id_moniter, sizeof(cpu_set_t), &cpuset);
    printf("\ntemperature_monitor %u thread on core %d \n",pthread_self(),sched_getcpu());

    while(finish!=1)
    {
        monitor_temperature();
        std::this_thread::sleep_for(std::chrono::microseconds(50000));
    }
}

void print_tempMap(){
    
    for(auto elem : tempMap)
    {
        fprintf(tmp_result,"%llu %d %d %d %d %d\n",elem.first, elem.second->sensors[0],elem.second->sensors[1],elem.second->sensors[2],elem.second->sensors[3],elem.second->sensors[4] );
    }

}

int decrease_core_temp(int init_temp, int core)
{    
    while(1)
    {
        int tmp=check_core_temperature(core-4);
        printf("########Current temperature at core %d: %d###########\n",core,tmp);
        if(tmp>init_temp)
            sleep(5);
        else
            return 1;
    }
    
}


void array_randomize(void* data, int size) {
    
    int i;
    
    DATATYPE* A;
    A = (DATATYPE*)data;
    for (i = 0; i < size; ++i)
    {
        A[i] = static_cast <DATATYPE> (rand()) / (static_cast <DATATYPE> (RAND_MAX/500)) - 250.0;
    }
    
}


void * multiply(void * arguments) 
{ 
   
    printf("Entered multiply thread\n");
    struct arg_struct *args = (struct arg_struct*)arguments;
    DATATYPE* mat1=args->arg1;
    DATATYPE* mat2=args->arg2;
    DATATYPE* res=args->arg3;
    int core=args->arg4;

    cpu_set_t cpuset;
    pthread_t thread_id_moniter = pthread_self(); 
    CPU_ZERO(&cpuset);
    CPU_SET(core, &cpuset);    
    int s = pthread_setaffinity_np(thread_id_moniter, sizeof(cpu_set_t), &cpuset);
    printf("\nmultiply %u thread on core %d \n",pthread_self(),sched_getcpu());
    // printf("multiply arguments: %u %u %u %d\n",mat1,mat2,res,core);
    int i, j, k; 
    for (i = 0; i < N; i++) 
    { 
        for (j = 0; j < N; j++) 
        { 
            res[i*N+j] = 0; 
            for (k = 0; k < N; k++) 
                res[i*N+j] += mat1[i*N+k]*mat2[k*N+j]; 
        } 
    }     
} 

void print_matix(DATATYPE* res)
{
    printf("Result matrix is \n"); 
    for (int i = 0; i < N; i++) 
    { 
        for (int j = 0; j < N; j++) 
           printf("%.2f ", res[i*N+j]); 
        printf("\n"); 
    } 
}

  
int main(int argc, char const *argv[])
{   

    int core=7;
    cpu_set_t cpuset;
    pthread_t thread_main = pthread_self(); 
    CPU_ZERO(&cpuset);
    CPU_SET(3, &cpuset);    
    int s = pthread_setaffinity_np(thread_main, sizeof(cpu_set_t), &cpuset);
    printf("\nmain thread %u on core %d \n",pthread_self(),sched_getcpu());


    char op_file[200]; 
    time_t t;
    time(&t);
    sprintf(op_file,"./output/tmp_MM_%s",ctime(&t));
    char *r = op_file;
    for (; *r; ++r)
    {
        if (*r == ' ' || *r == '\n')
              *r = '_';
    }
    tmp_result=fopen(op_file, "w+");
    printf("temperature dumped at %s\n",op_file);
    
    DATATYPE mat1_1[N][N], mat2_1[N][N],res_1[N][N];
    array_randomize(mat1_1, N*N);
    array_randomize(mat2_1, N*N);

    DATATYPE mat1_2[N][N], mat2_2[N][N],res_2[N][N];
    array_randomize(mat1_2, N*N);
    array_randomize(mat2_2, N*N);

    DATATYPE mat1_3[N][N], mat2_3[N][N],res_3[N][N];
    array_randomize(mat1_3, N*N);
    array_randomize(mat2_3, N*N);

    DATATYPE mat1_4[N][N], mat2_4[N][N],res_4[N][N];
    array_randomize(mat1_4, N*N);
    array_randomize(mat2_4, N*N);


    struct arg_struct* args_1;
    struct arg_struct* args_2;
    struct arg_struct* args_3;
    struct arg_struct* args_4;

    args_1 = (struct arg_struct *)malloc(sizeof(struct arg_struct));
    args_1->arg1 = (DATATYPE*)mat1_1;
    args_1->arg2 = (DATATYPE*)mat2_1;
    args_1->arg3 = (DATATYPE*)res_1;
    args_1->arg4 = (MULTICORE?4:core);

    args_2 = (struct arg_struct *)malloc(sizeof(struct arg_struct));
    args_2->arg1 = (DATATYPE*)mat1_2;
    args_2->arg2 = (DATATYPE*)mat2_2;
    args_2->arg3 = (DATATYPE*)res_2;
    args_2->arg4 = (MULTICORE?5:core);

    args_3 = (struct arg_struct *)malloc(sizeof(struct arg_struct));
    args_3->arg1 = (DATATYPE*)mat1_3;
    args_3->arg2 = (DATATYPE*)mat2_3;
    args_3->arg3 = (DATATYPE*)res_3;
    args_3->arg4 = (MULTICORE?6:core);

    args_4 = (struct arg_struct *)malloc(sizeof(struct arg_struct));
    args_4->arg1 = (DATATYPE*)mat1_4;
    args_4->arg2 = (DATATYPE*)mat2_4;
    args_4->arg3 = (DATATYPE*)res_4;
    args_4->arg4 = (MULTICORE?7:core);


    decrease_core_temp(58000,core);
    
    // sleep(10);
    struct timeval c_time;
    gettimeofday(&c_time,NULL); 
    START_TIME=(unsigned long long int )(c_time.tv_sec*1000000+c_time.tv_usec);
    pthread_t thread_temperature_monitor, thread_core1, thread_core2,thread_core3, thread_core4;
    pthread_create(&thread_temperature_monitor, NULL, temperature_monitor, NULL);
    finish=0;
    for(int k=0;k<10;k++)
    {   
        unsigned long long int start=get_current_time();      
        pthread_create(&thread_core1, NULL, multiply, args_1);
        if(!MULTICORE)
            pthread_join(thread_core1, NULL);
        pthread_create(&thread_core2, NULL, multiply, args_2);
        if(!MULTICORE)
            pthread_join(thread_core2, NULL);
        pthread_create(&thread_core3, NULL, multiply, args_3);
        if(!MULTICORE)
            pthread_join(thread_core3, NULL);
        pthread_create(&thread_core4, NULL, multiply, args_4);
        
        pthread_join(thread_core4, NULL);
        printf("***Execution time in multi thread: %llu ms.***\n",(get_current_time()-start)/1000);
    }   
        

    finish=1; 
   
    pthread_join(thread_temperature_monitor, NULL); 
    print_tempMap();

    // print_matix(args_4->arg3);
    
    fclose (tmp_result);
    printf("FINISHED!\n\n"); 
    return 0; 
} 

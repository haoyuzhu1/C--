#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <map>
#include <vector>
#include <random>
#include <chrono>

using namespace std;

//global constants:
int const MAX_NUM_OF_CHAN = 6;
int const MAX_NUM_OF_THREADS = 6;
int const DATA_BLOCK_SIZE = 20;

//global variables:
std::mutex mu; //declare a mutex
std::map<std::thread::id, int> threadIDs;
int randomTimeMs = 0;

//function prototypes: (if used)
int getThreadID();
void delays();
void showSamples(int id, double sampleFromADC);


class AdcInputChannel {
public:
AdcInputChannel(int d) //constructor
: currentSample(d) {} //constructor syntax for 'currentSample=d'
//used to request a sample from the sample channel:
double getCurrentSample() {
return 2*currentSample;
}
private:
int currentSample;
}; //end class AdcInputChannel

class Lock {
public:
Lock(): open(true) {} //constructor
//returns a flag to indicate when a thread should be blocked:
void lockLock(){
open = 0;
}
bool lock() {
return open;
}
void unlock() {
open = 1;
}
private:
bool open;
}; //end class Lock


class ADC {
public:
//constructor: initialises a vector of ADCInputChannels
//passed in by reference:
ADC(std::vector<AdcInputChannel>& channels): adcChannels(channels) {}
void requestADC(int c) {
std::unique_lock<std::mutex> locker(ADC_mu);
while(theADCLock.lock()==0){
cout << "ADC is locked, thread " << getThreadID() << " is about to suspend.." << endl;
cond.wait(locker);
}

//std::unique_lock<std::mutex> locker(mu);
theADCLock.lockLock();
cout << "ADC locked by thread " << getThreadID() << endl;
sampleChannel = c;
//locker.unlock(); //we're done, unlock core code
}
double sampleADC() {
return adcChannels[sampleChannel].getCurrentSample();
}
void releaseADC() {
std::unique_lock<std::mutex> locker(ADC_mu);
cout << "ADC unlocked by thread " << getThreadID() << endl;
theADCLock.unlock();
cond.notify_all();
}
private:
Lock theADCLock;
int sampleChannel;
std::vector<AdcInputChannel>& adcChannels;
std::mutex ADC_mu; //mutex

std::condition_variable cond;

};

void run(ADC& theADC, int id) {
std::unique_lock<std::mutex> map_locker(mu); //lock the map via the mutex.
//insert the threadID and id into the map:
threadIDs.insert(std::make_pair(std::this_thread::get_id(), id));
map_locker.unlock(); //we're done, unlock the map.

//to store the sampled data: (for part A2 only)
double sampleBlock[DATA_BLOCK_SIZE] = {0.0}; //initialise all elements to 0
for (int i=0; i<50; i++) { //replace with 'i<DATA_BLOCK_SIZE;' for A2
// request use of the ADC; channel to sample is id:
theADC.requestADC(id);
// sample the ADC:

double sampleFromADC = theADC.sampleADC();
showSamples(id, sampleFromADC);
// release the ADC:
theADC.releaseADC();

delays();
}
}

int getThreadID(){
std::map <std::thread::id, int>::iterator it = threadIDs.find(std::this_thread::get_id());
if (it == threadIDs.end()) return -1; //thread 'id' NOT found
else return it->second; //thread 'id' found, return the
}
void delays(){
std::mt19937 gen(time(0)); //A 'Mersenne_twister_engine' seeded by time(0).
//time(0) gives the number of seconds elapsed since the start
//of the world according to Unix (00:00:00 on 1st January 1970).
std::uniform_int_distribution<> dis(100, 500); //generate a random integer
//between 1-5.
randomTimeMs = dis(gen);
std::this_thread::sleep_for (std::chrono::milliseconds(randomTimeMs));
}
void showSamples(int id, double sampleFromADC){
std::unique_lock<std::mutex> locker(mu);
cout << " sample value form thread " << id ;
cout << " = " << sampleFromADC << endl;
}

int main() {
//initialise the ADC channels:
std::vector<AdcInputChannel> adcChannels;

for (int i = 0; i < MAX_NUM_OF_CHAN; i++) {
adcChannels.push_back(i);
}
// Instantiate the ADC:
ADC adcObj(adcChannels);
//instantiate and start the threads:
std::thread the_threads[MAX_NUM_OF_THREADS]; //array of threads
for (int i = 0; i < MAX_NUM_OF_THREADS; i++) {
//launch the threads:
the_threads[i] = std::thread(run,std::ref(adcObj), i);
}

for (int i = 0; i < MAX_NUM_OF_THREADS; i++) {
//launch the threads:
the_threads[i].join();
}

cout << "All threads terminated" << endl;
return 0;
}
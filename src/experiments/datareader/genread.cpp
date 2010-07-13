#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <pthread.h>

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/format.hpp>

#include <Lintel/ProgramOptions.hpp>
#include <Lintel/PThread.hpp>

using lintel::ProgramOption;
using namespace std;

#define BUF_SIZE 65536
#define MAX_READS 1000000  //Must be (sometimes a lot) greater than TOT_SIZE / BUF_SIZE
#define NETBAR "/home/krevate/projects/DataSeries/experiments/neta2a/net_call_bar pds-10"
#define NETBARSERVERS "/home/krevate/projects/DataSeries/experiments/neta2a/net_call_bar pds-11"
#define PORT_BASE 6000

typedef boost::shared_ptr<PThread> PThreadPtr;

ProgramOption<int> po_nodeIndex("node-index", "Node index in the node list", -1);
ProgramOption<long> po_dataAmount("data-amount", "Total amount of data to read from all hosts", 4000000000);
ProgramOption<string> po_nodeNames("node-names", "List of nodes");
static const string LOG_DIR("/home/krevate/projects/DataSeries/experiments/neta2a/logs/");
struct timeval *startTime, *jobEndTime;
vector<string> nodeNames;
int nodeIndex;

long tval2long(struct timeval *tval) {
    return ((tval->tv_sec*1000000) + tval->tv_usec);
}

long tval2longdiff(struct timeval *tvalstart, struct timeval *tvalend) {
    return (((tvalend->tv_sec-tvalstart->tv_sec)*1000000) + (tvalend->tv_usec-tvalstart->tv_usec));
}

class ReadThread : public PThread {
public:
    ReadThread(long dataAmount, int sockid, FILE *outlog)
	: dataAmount(dataAmount), sockid(sockid), outlog(outlog) {
	readDoneTime = (struct timeval*) malloc(sizeof(struct timeval));
	localEndTime = (struct timeval*) malloc(sizeof(struct timeval));	
    }

    virtual ~ReadThread() {
	free(readDoneTime);
	free(localEndTime);
    }

    virtual void *run() {
	long *retSizePtr;
	retSizePtr = (long *)malloc(sizeof(long));
	*retSizePtr = 42; //runReader();
	printReadTimes();
	fprintf(outlog, "Local work finished: %ld us\n", tval2longdiff(startTime, localEndTime) );
	return retSizePtr;
    }

    void printReadTimes() {
	int i;
	long totReadTime = 0;
	long totReadAmnt = 0;
	fprintf(outlog, "\nReadTime ReadAmnt\n");
	for (i = 0; i < totReads; i++) {
	    fprintf(outlog, "%ld %ld\n", readTimes[i], readAmounts[i]);
	    totReadAmnt += readAmounts[i];
	    totReadTime += readTimes[i];
	}
	fprintf(outlog, "Total number of reads: %ld\n", totReads);
	fprintf(outlog, "Total amount read: %ld\n", totReadAmnt);
	fprintf(outlog, "Total time in reads: %ld us\n", totReadTime);
    }
        
    long runReader() {
	int ret;
	long tot = 0;
	long thisReadTime = 0;
	long lastReadTime = 0;
	long numReads = 0;
	totReads = 0;
	
	for (ret = 0, numReads = 0, lastReadTime = tval2long(startTime); tot < dataAmount; ++numReads, tot += ret) {
	    // Note: always reading buf_size is ok because we control exact amount sent
	    ret = read(sockid, buf, BUF_SIZE);
	    gettimeofday(readDoneTime, NULL);
	    readAmounts[numReads] = ret;
	    thisReadTime = tval2long(readDoneTime);
	    readTimes[numReads] = thisReadTime - lastReadTime;
	    lastReadTime = thisReadTime;
	    if (ret == 0) {
		break;
	    }
	}
	
	totReads = ++numReads;
	gettimeofday(localEndTime, NULL);
	
	return tot;
    }

    struct timeval *readDoneTime, *localEndTime;
    char buf[BUF_SIZE];
    long readTimes[MAX_READS]; //time between successful reads
    long readAmounts[MAX_READS]; //amount of data actually read each time
    long totReads; //total reads taken to read all data
    long dataAmount; //amount of data to read from socket
    int sockid; //id of socket to read from
    FILE *outlog; //output file id
};

class SetupAndTransferThread : public PThread {

public:

    SetupAndTransferThread(long dataAmount, unsigned short int serverPort, int isServer, int otherNodeIndex)
	: dataAmount(dataAmount), serverPort(serverPort), isServer(isServer), otherNodeIndex(otherNodeIndex) {}

    ~SetupAndTransferThread() {}

    virtual void *run() {
	long *retSizePtr = 0;
	int sockid = 0;
	PThreadPtr readThread;
	FILE *outlog;

	// Open output file for log
	string outlogfile = (boost::format("%s%d.%d.%s.test") % LOG_DIR %
			     (isServer == 0 ? otherNodeIndex : nodeIndex) % 
			     (isServer == 0 ? nodeIndex : otherNodeIndex) % 
			     (isServer == 0 ? "c" : "s")).str();
	cout << outlogfile;
	printf("%d: isServer? %d otherNodeIndex: %d\n", nodeIndex, isServer, otherNodeIndex);
	outlog = fopen(outlogfile.c_str(), "w+");
	if (outlog == NULL) {
	    fprintf(stderr, "ERROR opening outlog");
	}

	// Connect or listen as per the args, getting a sockid
	sockid = 0;
	
	// Start up the data reader
	readThread.reset(new ReadThread(dataAmount, sockid, outlog));
	readThread->start();

	// Generate and send data over the connection

	// Join all read threads
	retSizePtr = (long *)readThread->join();
	
	fclose(outlog);
	return retSizePtr;
    }

    long dataAmount;
    unsigned short int serverPort;
    int isServer;
    int otherNodeIndex;
};

int main(int argc, char **argv)
{
    lintel::parseCommandLine(argc,argv);
    long dataAmount = 0;
    long dataAmountPerNode = 0;
    string tmp;
    int numNodes = 0;
    int isServer = 0;
    long *retSizePtr = 0;
    long totReturned = 0;
    PThreadPtr *setupAndTransferThreads; //will hold array of pthread pointers, one for each nodeindex
    //string serverName = "pds-10.u.hpl.hp.com";
    //struct sockaddr_in serverAddress;
    //struct hostent *hostInfo;

    startTime = (struct timeval*) malloc(sizeof(struct timeval));
    jobEndTime = (struct timeval*) malloc(sizeof(struct timeval));        
    dataAmount = po_dataAmount.get();
    nodeIndex = po_nodeIndex.get();
    tmp = po_nodeNames.get();
    boost::split(nodeNames, tmp, boost::is_any_of(","));
    numNodes = nodeNames.size();
    dataAmountPerNode = dataAmount / numNodes;
    SINVARIANT(numNodes >= 0);
    setupAndTransferThreads = (PThreadPtr *) malloc(numNodes*sizeof(PThreadPtr));
    printf("\ngenread called with %d nodes and %ld Bytes\n",numNodes,dataAmount);

    system(NETBARSERVERS);
    gettimeofday(startTime, NULL);

    // Specify which connections apply to this node, as a server or client
    for (int i = (numNodes - 1); i >= 0; --i) {
	if (i == nodeIndex) {
	    // We finished setting up servers, wait for all other servers to go up
	    printf("\n%d: finished setting up servers\n", nodeIndex);
	    system(NETBARSERVERS);
	} else {
	    // Determine if this node acts as server or client
	    if (i > nodeIndex) {
		isServer = 1;
	    } else {
		isServer = 0;
	    }
	    setupAndTransferThreads[i].reset(new SetupAndTransferThread(dataAmountPerNode, PORT_BASE+i, isServer, i));
	    setupAndTransferThreads[i]->start();
	}
    }

    // Join all threads
    for (int i = 0; i < numNodes; i++) {
	if (i == nodeIndex) {
	    ; // Do nothing
	} else {
	    retSizePtr = (long *)setupAndTransferThreads[i]->join();
	    printf("\nData received from nodeindex %d: %ld\n", i, *retSizePtr);
	    totReturned += *retSizePtr;
	    free(retSizePtr);
	}
    }

    printf("\nTotal received: %ld\n", totReturned);
    system(NETBARSERVERS);
    
    gettimeofday(jobEndTime, NULL);    
    printf("Full job finished:   %ld us\n", tval2longdiff(startTime, jobEndTime) );

    free(startTime);
    free(jobEndTime);
}

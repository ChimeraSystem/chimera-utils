#include <chimera_network.h>
#include <chimera_telemetry.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

extern int64_t chimera_syscall(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t);
static int snapshot(unsigned index,chimera_interface_stats_t *info) {
    memset(info,0,sizeof(*info));
    int64_t result=chimera_syscall(SYS_sysinfo,(uintptr_t)info,CHIMERA_SYSINFO_INTERFACE_STATS,
                   index,sizeof(*info),CHIMERA_INTERFACE_STATS_VERSION,0);
    if (result<0) {errno=(int)-result;return -1;}
    return 0;
}
static void address(const char *label,uint32_t value) {
    char text[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET,&value,text,sizeof(text))) printf(" %s %s",label,text);
}
static void show(const chimera_interface_stats_t *info,int verbose) {
    const chimera_interface_info_t *net=&info->interface;
    static const struct {unsigned flag;const char *name;} flags[]={
        {0x1,"UP"},{0x2,"BROADCAST"},{0x4,"DEBUG"},{0x8,"LOOPBACK"},{0x40,"RUNNING"},{0x8000,"MULTICAST"}};
    printf("%s: flags=%x<",net->name,net->flags);
    int comma=0;
    for (unsigned i=0;i<sizeof(flags)/sizeof(flags[0]);i++) if (net->flags&flags[i].flag) {
        printf("%s%s",comma ? "," : "",flags[i].name);comma=1;
    }
    printf("> mtu %u\n",net->mtu);
    if (!(net->flags&0x8)) printf("\tether %02x:%02x:%02x:%02x:%02x:%02x\n",
        net->mac[0],net->mac[1],net->mac[2],net->mac[3],net->mac[4],net->mac[5]);
    if (net->ipv4) {
        printf("\t");address("inet",net->ipv4);address("netmask",net->netmask);
        if (net->flags&0x2) address("broadcast",info->broadcast);
        putchar('\n');
    }
    printf("\tstatus: %s\n",!(net->flags&0x1) ? "down" : net->flags&0x40 ? "active" : "no carrier");
    if (verbose) {
        if (net->gateway || info->dns) {
            printf("\t");if (net->gateway) address("gateway",net->gateway);
            if (info->dns) address("dns",info->dns);putchar('\n');
        }
        printf("\tRX packets %llu bytes %llu errors %llu\n",
            (unsigned long long)info->rx_packets,(unsigned long long)info->rx_bytes,(unsigned long long)info->rx_errors);
        printf("\tTX packets %llu bytes %llu errors %llu\n",
            (unsigned long long)info->tx_packets,(unsigned long long)info->tx_bytes,(unsigned long long)info->tx_errors);
    }
}
static int usage(void) {
    fputs("usage: ifconfig [-a] [-v] [interface]\n"
          "       ifconfig -l\n"
          "       ifconfig interface up | down | dhcp\n"
          "       ifconfig interface inet address netmask mask [gateway address] [dns address]\n",stderr);
    return 64;
}
int main(int argc,char **argv) {
    int verbose=0,names=0,index=1;
    while (index<argc && argv[index][0]=='-') {
        if (!strcmp(argv[index],"-a")) {}
        else if (!strcmp(argv[index],"-v")) verbose=1;
        else if (!strcmp(argv[index],"-l")) names=1;
        else return usage();
        index++;
    }
    const char *name=index<argc ? argv[index++] : NULL;
    if (name && (strlen(name)>=16 || names)) return usage();
    if (index<argc) {
        if (verbose || names || !name) return usage();
        chimera_network_status status;
        chimera_network_config_t config={.version=CHIMERA_NETWORK_CONFIG_V1,.size=sizeof(config)};
        memcpy(config.name,name,strlen(name)+1);
        unsigned command;
        if (index+1==argc && (!strcmp(argv[index],"up") || !strcmp(argv[index],"down"))) {
            command=CHIMERA_NET_ENABLE;config.up=!strcmp(argv[index],"up");
        } else if (index+1==argc && !strcmp(argv[index],"dhcp")) command=CHIMERA_NET_DHCP;
        else if (!strcmp(argv[index],"inet") && index+3<argc && !strcmp(argv[index+2],"netmask")) {
            command=CHIMERA_NET_MANUAL;config.up=1;
            if (inet_pton(AF_INET,argv[index+1],&config.ipv4)!=1 ||
                inet_pton(AF_INET,argv[index+3],&config.netmask)!=1) return usage();
            unsigned seen=0;
            for (index+=4;index<argc;index+=2) {
                if (index+1==argc) return usage();
                uint32_t *field;unsigned bit;
                if (!strcmp(argv[index],"gateway")) {field=&config.gateway;bit=1;}
                else if (!strcmp(argv[index],"dns")) {field=&config.dns;bit=2;}
                else return usage();
                if ((seen&bit) || inet_pton(AF_INET,argv[index+1],field)!=1) return usage();
                seen|=bit;
            }
        } else return usage();
        if (chimera_network_call(command,&config,&status)) {perror("ifconfig: networkd");return 1;}
        return 0;
    }
    int count=0;
    for (unsigned i=0;i<256;i++) {
        chimera_interface_stats_t info;
        if (snapshot(i,&info)) {
            if (errno==ENOENT) break;
            perror("ifconfig: interface snapshot");return 1;
        }
        if (info.version!=CHIMERA_INTERFACE_STATS_VERSION || info.size!=sizeof(info)) {
            fputs("ifconfig: unsupported interface snapshot\n",stderr);return 1;
        }
        if (name && strcmp(name,info.interface.name)) continue;
        if (names) printf("%s%s",count ? " " : "",info.interface.name);
        else show(&info,verbose);
        count++;
        if (name) break;
    }
    if (names) putchar('\n');
    if (name && !count) {fprintf(stderr,"ifconfig: no such interface: %s\n",name);return 1;}
    return 0;
}

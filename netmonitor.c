#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <stdio.h>
#include <string.h>
#include <net/if.h>

int main() {
	
	/*
	 Tracking RX/TX 
	 */

	struct sockaddr_nl addr;
	int nl_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	char nlbuffer[4096];
	memset(&addr, 0, sizeof(addr));
	memset(nlbuffer, 0, sizeof(nlbuffer));
	struct nlmsghdr *nlh = (struct nlmsghdr *)nlbuffer;

	nlh->nlmsg_type = RTM_GETLINK;
	nlh->nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
	nlh->nlmsg_seq = 1;
	nlh->nlmsg_pid = 0;

	addr.nl_family = AF_NETLINK;
	addr.nl_pid    = 0;
	addr.nl_groups = 0;

	if (nl_fd == -1) {
		printf("something went wrong when checking return value of nl_fd");
		perror("socket");
		return 1;
	}

	if (bind(nl_fd,
	   (struct sockaddr *)&addr,
  	   sizeof(addr)) == -1)	   
	{
		perror("bind");
		return 1;
	}
	

 	struct sockaddr_nl kernel;
		memset(&kernel, 0, sizeof(kernel));
		kernel.nl_family = AF_NETLINK;
	
	struct iovec iov = {
    		.iov_base = nlh,
    		.iov_len  = nlh->nlmsg_len
		};

	struct msghdr msg = {
    		.msg_name    = &kernel,
    		.msg_namelen = sizeof(kernel),
    		.msg_iov     = &iov,
    		.msg_iovlen  = 1
		};

	if (sendmsg(nl_fd, &msg, 0) == -1) {
    		perror("sendmsg");
    		return 1;
		}

	ssize_t len = recvmsg(nl_fd, &msg, 0);
	if (len == -1) {
    		perror("recvmsg");
    		return 1;
	}

	for (struct nlmsghdr *nh = (struct nlmsghdr *)nlbuffer;
     		NLMSG_OK(nh, len);
     		nh = NLMSG_NEXT(nh, len))
	{
    		if (nh->nlmsg_type == NLMSG_DONE) {
			break;
		}

		if (nh->nlmsg_type == NLMSG_ERROR) {
			struct nlmsgerr *err = (struct nlmsgerr *)NLMSG_DATA(nh);
			if (err->error != 0) {
				fprintf(stderr, "netlink error: %d\n", err->error);
				return 1;
			}
			continue;
		}

		struct ifinfomsg *ifi = (struct ifinfomsg *)NLMSG_DATA(nh);
		struct rtattr *attr = (struct rtattr *)((char *)ifi + NLMSG_ALIGN(sizeof(*ifi)));
		int attr_len = nh->nlmsg_len - NLMSG_LENGTH(sizeof(*ifi));

		for (; RTA_OK(attr, attr_len); attr = RTA_NEXT(attr, attr_len)) {
			if (attr->rta_type == IFLA_STATS64) {
				char ifname[IF_NAMESIZE];
				if_indextoname(ifi->ifi_index, ifname);

				struct rtnl_link_stats64 *stats = 
					(struct rtnl_link_stats64 *)RTA_DATA(attr);

				unsigned long long rx = stats->rx_packets;
				unsigned long long tx = stats->tx_packets;

				printf("RX packets: %llu, TX packets: %llu\n", rx, tx);
			}	
		}
	}






	printf("hello netlink monitor\n");
	return 0;
		
  }

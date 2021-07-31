#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/connector.h>
#include <linux/cn_proc.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h> /* for wait */

#include "support.c"

pid_t wrapper_PID = 0;

FILE *fp;

/*
* connect to netlink
* returns netlink socket, or -1 on error
*/
static int nl_connect()
{
int rc;
int nl_sock;
struct sockaddr_nl sa_nl;

nl_sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR);
if (nl_sock == -1) {
perror("socket");
return -1;
}

sa_nl.nl_family = AF_NETLINK;
sa_nl.nl_groups = CN_IDX_PROC;
sa_nl.nl_pid = getpid();

rc = bind(nl_sock, (struct sockaddr *)&sa_nl, sizeof(sa_nl));
if (rc == -1) {
perror("bind");
close(nl_sock);
return -1;
}

return nl_sock;
}

/*
* subscribe on proc events (process notifications)
*/
static int set_proc_ev_listen(int nl_sock, bool enable)
{
int rc;
struct __attribute__ ((aligned(NLMSG_ALIGNTO))) {
struct nlmsghdr nl_hdr;
struct __attribute__ ((__packed__)) {
struct cn_msg cn_msg;
enum proc_cn_mcast_op cn_mcast;
};
} nlcn_msg;

memset(&nlcn_msg, 0, sizeof(nlcn_msg));
nlcn_msg.nl_hdr.nlmsg_len = sizeof(nlcn_msg);
nlcn_msg.nl_hdr.nlmsg_pid = getpid();
nlcn_msg.nl_hdr.nlmsg_type = NLMSG_DONE;

nlcn_msg.cn_msg.id.idx = CN_IDX_PROC;
nlcn_msg.cn_msg.id.val = CN_VAL_PROC;
nlcn_msg.cn_msg.len = sizeof(enum proc_cn_mcast_op);

nlcn_msg.cn_mcast = enable ? PROC_CN_MCAST_LISTEN : PROC_CN_MCAST_IGNORE;

rc = send(nl_sock, &nlcn_msg, sizeof(nlcn_msg), 0);
if (rc == -1) {
perror("netlink send");
return -1;
}

return 0;
}

/*
* handle a single process event
*/
static volatile bool need_exit = false;
static int handle_proc_ev(int nl_sock)
{
int rc;
	struct __attribute__ ((aligned(NLMSG_ALIGNTO))) {
	struct nlmsghdr nl_hdr;
	struct __attribute__ ((__packed__)) {
	struct cn_msg cn_msg;
	struct proc_event proc_ev;
	};
} nlcn_msg;



while (!need_exit) {

char cmdline[1024], ids[1024];
rc = recv(nl_sock, &nlcn_msg, sizeof(nlcn_msg), MSG_WAITALL);

if (rc == 0) {
/* shutdown? */
//continue;
return 0;
} else if (rc == -1) {
if (errno == EINTR) continue;
perror("netlink recv");
return -1;
}
// printf("Here");
switch (nlcn_msg.proc_ev.what) {
	case PROC_EVENT_NONE:
	//printf("set mcast listen ok\n");
	break;
	case PROC_EVENT_FORK:
		//printf("FORK: PID=%d PPID=%d \n", nlcn_msg.proc_ev.event_data.fork.child_pid, nlcn_msg.proc_ev.event_data.fork.parent_pid);
		fprintf(fp, "FORK: PID=%d PPID=%d \n", nlcn_msg.proc_ev.event_data.fork.child_pid, nlcn_msg.proc_ev.event_data.fork.parent_pid);
	break;
	case PROC_EVENT_EXEC:
	{
		extract_ids_cmdline(nlcn_msg.proc_ev.event_data.exec.process_pid, cmdline, sizeof(cmdline), ids, sizeof(ids));
		//printf("EXEC: PID=%d PPID=%d\n\n", nlcn_msg.proc_ev.event_data.exec.process_pid, nlcn_msg.proc_ev.event_data.fork.parent_pid);
		fprintf(fp, "EXEC: PID=%d CMD=%s\n", nlcn_msg.proc_ev.event_data.exec.process_pid, cmdline);
		/*printf("exec: tid=%d pid=%d parent=%d \t[%s]\t[%s] \n\n",
		nlcn_msg.proc_ev.event_data.exec.process_pid,
		nlcn_msg.proc_ev.event_data.exec.process_tgid, nlcn_msg.proc_ev.event_data.fork.parent_pid, ids, cmdline);
		*/
	
	break;
	}
	case PROC_EVENT_UID:
	
	break;
	case PROC_EVENT_GID:
	
	break;
	case PROC_EVENT_EXIT:
	
	default:
	//printf("unhandled proc event\n");
	break;
	}
}

return 0;
}

static void on_sigint(int unused)
{
need_exit = true;
fclose(fp);
}

int main(int argc, const char *argv[])
{
wrapper_PID = getpid();
fp = fopen("log.txt", "w");
int nl_sock;
int rc = EXIT_SUCCESS;

signal(SIGINT, &on_sigint);
siginterrupt(SIGINT, true);

nl_sock = nl_connect();
if (nl_sock == -1)
exit(EXIT_FAILURE);

rc = set_proc_ev_listen(nl_sock, true);
if (rc == -1) {
rc = EXIT_FAILURE;
goto out;
}

rc = handle_proc_ev(nl_sock);
if (rc == -1) {
rc = EXIT_FAILURE;
goto out;
}

set_proc_ev_listen(nl_sock, false);

out:
close(nl_sock);
exit(rc);
}


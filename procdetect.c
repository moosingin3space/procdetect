#include <sys/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/connector.h>
#include <linux/cn_proc.h>

#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <errno.h>
#include <stdlib.h>

#include "proc_tbl.h"

#define NL_MESSAGE_SIZE (sizeof(struct nlmsghdr) + sizeof(struct cn_msg) + sizeof(int))
#define MAX_PROC_BUCKETS 2048

static int nl_sock;
static proc_t *g_procs[MAX_PROC_BUCKETS];

int connect_to_netlink()
{
    // Socket specs
    struct sockaddr_nl sa_nl;
    char buffer[NL_MESSAGE_SIZE];
    struct nlmsghdr *hdr;
    struct cn_msg *msg;

    // Establish connection
    nl_sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR);

    if (nl_sock == -1) {
        perror("failed to connect to netlink");
        return errno;
    }

    // Bind socket
    bzero(&sa_nl, sizeof(sa_nl));
    sa_nl.nl_family = AF_NETLINK;
    sa_nl.nl_groups = CN_IDX_PROC;
    sa_nl.nl_pid = getpid();

    if (bind(nl_sock, (struct sockaddr *)&sa_nl, sizeof(sa_nl)) == -1) {
        perror("failed to bind to netlink socket");
        return errno;
    }

    // Register for process events on the netlink socket

    // Set up header
    hdr = (struct nlmsghdr *)buffer;
    hdr->nlmsg_len = NL_MESSAGE_SIZE;
    hdr->nlmsg_type = NLMSG_DONE;
    hdr->nlmsg_flags = 0;
    hdr->nlmsg_seq = 0;
    hdr->nlmsg_pid = getpid();

    // Set up message
    msg = (struct cn_msg *)NLMSG_DATA(hdr);
    msg->id.idx = CN_IDX_PROC;
    msg->id.val = CN_VAL_PROC;
    msg->seq = 0;
    msg->ack = 0;
    msg->flags = 0;
    msg->len = sizeof(int);
    *(int*)msg->data = PROC_CN_MCAST_LISTEN;

    if (send(nl_sock, hdr, hdr->nlmsg_len, 0) == -1) {
        perror("failed to register for process events");
        return errno;
    }

    return 0;
}

void process_start(pid_t pid)
{
    proc_t *p = malloc(sizeof(proc_t));
    time_t start;
    bool add;
    time(&start);
    add = proc_fill(pid, start, p);
    if (!add) {
        free(p);
        return;
    }
    proc_t *bucket = g_procs[pid % MAX_PROC_BUCKETS];
    g_procs[pid % MAX_PROC_BUCKETS] = insert_proc(bucket, p);
    printf("Process \"%s\" (pid %d) started\n",
            p->exe, p->pid);
}

void process_end(pid_t pid)
{
    time_t end;
    int duration;
    proc_t *bucket = g_procs[pid % MAX_PROC_BUCKETS];
    proc_t *p = get_proc(bucket, pid);
    if (p == NULL)
    {
        // Can't do anything
        return;
    }

    time(&end);
    duration = end - p->start_time;
    printf("Process \"%s\" (pid %d) exited after %d seconds\n",
            p->exe, p->pid, duration);
    g_procs[pid % MAX_PROC_BUCKETS] = remove_proc(bucket, pid);
}

void cleanup_processes()
{
    printf("Cleaning up...\n");
    for (size_t i = 0; i < MAX_PROC_BUCKETS; i++)
    {
        if (g_procs[i] != NULL)
        {
            proc_t *runner = g_procs[i];
            while (runner != NULL)
            {
                proc_t *next = runner->next;
                free(runner);
                runner = next;
            }
        }
    }
}

void handle_events_from_nlsock()
{
    char buffer[CONNECTOR_MAX_MSG_SIZE];
    struct nlmsghdr *hdr;
    struct proc_event *event;

    fd_set fds;

    while (1) {
        FD_ZERO(&fds);
        FD_SET(nl_sock, &fds);

        if (select(nl_sock + 1, &fds, NULL, NULL, NULL) < 0) {
            perror("select() failed");
            return;
        }

        if (!FD_ISSET(nl_sock, &fds)) {
            perror("no events detected");
            return;
        }

        if (recv(nl_sock, buffer, sizeof(buffer), 0) == -1) {
            perror("recv failed");
            return;
        }

        hdr = (struct nlmsghdr *)buffer;

        if (NLMSG_ERROR == hdr->nlmsg_type) {
            perror("NLMSG_ERROR");
        } else if (NLMSG_DONE == hdr->nlmsg_type) {
            event = (struct proc_event *)((struct cn_msg *)NLMSG_DATA(hdr))->data;

            switch (event->what) {
                case PROC_EVENT_EXIT:
                    process_end(event->event_data.exit.process_pid);
                    break;
                case PROC_EVENT_EXEC:
                    process_start(event->event_data.exec.process_pid);
                    break;

                default:
                    // ignore
                    break;
            }
        }
    }
}

int main(int argc, char **argv)
{
    bzero(g_procs, MAX_PROC_BUCKETS*sizeof(proc_t*));
    if (!connect_to_netlink()) {
        handle_events_from_nlsock();
    }
    cleanup_processes();
    return 0;
}

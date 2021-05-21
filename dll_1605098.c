#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: SLIGHTLY MODIFIED
 FROM VERSION 1.1 of J.F.Kurose

   This code should be used for PA2, unidirectional or bidirectional
   data transfer protocols (from A to B. Bidirectional transfer of data
   is for extra credit and is not required).  Network properties:
   - one way network delay averages five time units (longer if there
       are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
       or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
       (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 1 /* change to 1 if you're doing extra credit */
/* and write a routine called B_output */

/* a "pkt" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct pkt
{
    char data[4];
};
int aseq; // sequence no of messages A will send.
int bseq; // sequence no of messages B will send.
int aack; // acknowledge no of A's message receiving
int back; // acknowledge no of B's message receiving

struct frm p_Adata; // send from A side
struct frm p_Aack;
struct frm p_Bdata; // send from B side
struct frm p_Back;

int receivedMsgAck_A = 1; //defines whether the current process is finished or not for A
int receivedMsgAck_B = 1; // //defines whether the current process is finished or not for B

int outstandingACK_A = 0;
int outstandingACK_B = 0;

int piggybacking = 0; // 0 for no pb, 1 for pb.
/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct frm
{
    int ftype;
    int seqnum;
    int acknum;
    int checksum;
    char payload[4];
};

/********* FUNCTION PROTOTYPES. DEFINED IN THE LATER PART******************/
void starttimer(int AorB, float increment);
void stoptimer(int AorB);
void tolayer1(int AorB, struct frm packet);
void tolayer3(int AorB, char datasent[20]);

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

int calCheckSum(struct frm packet)
{
    int checkSum = 0;
    checkSum = packet.seqnum + packet.acknum;

    for(int i = 0; i < 4; i++)
    {
        checkSum += packet.payload[i];
    }
    return checkSum;
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct pkt message)
{
    printf("A_output: \n");

    if(receivedMsgAck_A == 0)
    {
        printf("Ongoing data transmission of A.\n");
        printf("Packet Dropped.\n");
        return;
    }

    p_Adata.ftype = 0;
    p_Adata.acknum = -1;

    if(piggybacking == 1 && outstandingACK_A == 1)
    {
        p_Adata.ftype = 2;
        //aseq = 1 - aseq;
        p_Adata.acknum = 1 - aack;//aseq;//
        outstandingACK_A = 0;
    }

    p_Adata.seqnum = aseq;

    strcpy(p_Adata.payload, message.data);
    p_Adata.checksum = calCheckSum(p_Adata);

    receivedMsgAck_A = 0; // started the process of A

    printf("   Sending: ftype: %d, seq: %d, ack: %d, csum: %d, data: %s \n",p_Adata.ftype, p_Adata.seqnum, p_Adata.acknum, p_Adata.checksum, p_Adata.payload);
    tolayer1(0, p_Adata);

    starttimer(0, 30);
}

/* need be completed only for extra credit */
void B_output(struct pkt message)
{
    printf("B_output: \n");

    if(receivedMsgAck_B == 0)
    {
        printf("Ongoing data transmission of B.\n");
        printf("Packet Dropped.\n");
        return;
    }

    p_Bdata.ftype = 0;
    p_Bdata.acknum = -1;

    if(piggybacking == 1 && outstandingACK_B == 1)
    {
        p_Bdata.ftype = 2;
        //bseq = 1 - bseq;
        p_Bdata.acknum = 1 - back;//bseq; //
        outstandingACK_B = 0;
    }

    p_Bdata.seqnum = bseq;

    strcpy(p_Bdata.payload, message.data);
    p_Bdata.checksum = calCheckSum(p_Bdata);

    receivedMsgAck_B = 0; // started the process

    printf("   Sending: ftype: %d, seq: %d, ack: %d, csum: %d, data: %s \n",p_Bdata.ftype, p_Bdata.seqnum, p_Bdata.acknum, p_Bdata.checksum, p_Bdata.payload);
    tolayer1(1, p_Bdata);

    starttimer(1, 30);
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct frm packet)
{
    printf("A_input: ");
    if (packet.ftype == 0 && packet.payload != 0) // received data from B side.
    {
        printf("      (Data received from B)\n");
        p_Aack.ftype = 1;
        if (packet.seqnum != aack)
        {
            printf("   Received Duplicate Packet.     Dropped.\n");
            printf("   Sending ack(prev):ftype: %d, seq: %d, ack: %d, csum: %d, data:%s\n",p_Aack.ftype, p_Aack.seqnum, p_Aack.acknum, p_Aack.checksum, p_Aack.payload);
            outstandingACK_A = 0;
            tolayer1(0, p_Aack);
        }
        else if (packet.checksum != calCheckSum(packet))
        {
            printf("   Packet Corrupted. \n   Sending ack(prev):ftype: %d, seq: %d, ack: %d, csum: %d, data:%s\n",p_Aack.ftype, p_Aack.seqnum, p_Aack.acknum, p_Aack.checksum, p_Aack.payload);
            tolayer1(0, p_Aack);
        }
        else
        {
            tolayer3(0,packet.payload);

            p_Aack.ftype = 1;
            p_Aack.seqnum = packet.seqnum;
            p_Aack.acknum = aack;
            strcpy(p_Aack.payload, "ACK");
            p_Aack.checksum = calCheckSum(p_Aack);

            printf("   Received successfully at A: ftype: %d, seq: %d, ack: %d, csum: %d, data: %s \n", packet.ftype, packet.seqnum, packet.acknum, packet.checksum, packet.payload);
            if (piggybacking == 0)
            {
                printf("   Sending Ack: ftype: %d, seq: %d, ack: %d, csum: %d, data: %s \n", p_Aack.ftype, p_Aack.seqnum, p_Aack.acknum, p_Aack.checksum, p_Aack.payload);
                tolayer1(0, p_Aack);
            }
            else
            {
                outstandingACK_A = 1;
                printf("Waiting for another message from A to B.\n");
            }
            aack = 1 - aack;
        }
    }
    else if( packet.ftype == 1 ) // receives ack at A.
    {
        printf("      (Ack received from B)\n");

        if ( packet.acknum != aseq)
        {
            printf("   Received Ack: ftype: %d, seq: %d, ack: %d, csum: %d, data: %s\n", packet.ftype, packet.seqnum, packet.acknum, packet.checksum, packet.payload);
            printf("   Ignoring Ack, as it is ack for previous packet.\n");
            return;
        }
        else if ( packet.checksum != calCheckSum(packet) )
        {
            printf("   Received Ack(corrupted): ftype: %d, seq: %d, ack: %d, csum: %d, data: %s\n",packet.ftype, packet.seqnum, packet.acknum, packet.checksum, packet.payload);
        }
        else
        {
            printf("   Acknowledgment received Successfully. \n");
            printf("   Received Ack: ftype: %d, seq: %d, ack: %d, csum: %d, data: %s\n",packet.ftype, packet.seqnum, packet.acknum, packet.checksum, packet.payload);

            receivedMsgAck_A = 1; // full process ended
            aseq = 1 - aseq;

            stoptimer(0);
            printf("Whole process finished for the current message of A-->B. \n\n\n");
        }
    }
    else if (packet.ftype == 2) // received data + piggyback ack
    {
        printf("      (Data + Ack received from B)\n");
        if(packet.acknum == aseq )
        {
            printf("   Acknowledgment received Successfully. \n");
            receivedMsgAck_A = 1; // full process ended
            aseq = 1 - aseq;

            stoptimer(0);
            printf("Whole process finished for the current message of A-->B. \n\n\n");
        }

        p_Aack.ftype = 1;
        if (packet.seqnum != aack)
        {
            printf("   Received Duplicate Packet.     Dropped.\n");
            if(piggybacking == 1 && outstandingACK_A == 1)
            {
                printf("   Sending Ack: ftype: %d, seq: %d, ack: %d, csum: %d, data: %s \n",p_Aack.ftype, p_Aack.seqnum, p_Aack.acknum, p_Aack.checksum, p_Aack.payload);
                tolayer1(0, p_Aack);

                //aack = 1 - aack;
                outstandingACK_A = 0;
            }
        }
        else if (packet.checksum != calCheckSum(packet))
        {
            printf("   Packet Corrupted. \n   Sending ack(prev): ftype: %d, seq: %d, ack: %d, csum: %d, data:%s\n",p_Aack.ftype, p_Aack.seqnum, p_Aack.acknum, p_Aack.checksum, p_Aack.payload);
            tolayer1(0, p_Aack);
        }
        else
        {
            tolayer3(0,packet.payload);
            printf("   Received successfully at A: ftype: %d, seq: %d, ack: %d, csum: %d, data: %s \n",packet.ftype, packet.seqnum, packet.acknum, packet.checksum, packet.payload);

            p_Aack.ftype = 1;
            p_Aack.seqnum = packet.seqnum;
            p_Aack.acknum = back;
            strcpy(p_Aack.payload, "ACK");
            p_Aack.checksum = calCheckSum(p_Aack);

            if (piggybacking == 0 )
            {
                printf("   Sending Ack: ftype: %d, seq: %d, ack: %d, csum: %d, data: %s \n",p_Aack.ftype, p_Aack.seqnum, p_Aack.acknum, p_Aack.checksum, p_Aack.payload);
                tolayer1(0, p_Aack);
            }
            else
            {
                outstandingACK_A = 1;
                printf("Waiting for another message from A to B.\n");
            }
            aack = 1 - aack;
        }
    }
}

/* called when A's timer goes off */
void A_timerinterrupt(void)
{
    if(receivedMsgAck_A == 0 )
    {
        printf("   A's Timer Interrupt is called. Resending message:\n");
        printf("   Resending: ftype: %d, seq: %d, ack: %d, csum: %d, data: %s \n", p_Adata.ftype, p_Adata.seqnum, p_Adata.acknum, p_Adata.checksum, p_Adata.payload);
        tolayer1(0, p_Adata);
        starttimer(0, 30);
    }
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init(void)
{
    aseq = 0;
    aack = 0;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct frm packet)
{
    printf("B_input: ");

    if( packet.ftype == 0 && packet.payload != 0 ) // received data from sender A.
    {
        printf("      (Data received from A)\n");
        p_Back.ftype = 1;
        if (packet.seqnum != back)
        {
            printf("   Received Duplicate Packet.     Dropped.\n");
            printf("   Sending ack(prev): ftype: %d, seq: %d, ack: %d, csum: %d, data:%s\n",p_Back.ftype, p_Back.seqnum, p_Back.acknum, p_Back.checksum, p_Back.payload);
            outstandingACK_B = 0;
            tolayer1(1, p_Back);
        }
        else if (packet.checksum != calCheckSum(packet))
        {
            printf("   Packet Corrupted. \n   Sending ack(prev): ftype: %d, seq: %d, ack: %d, csum: %d, data:%s\n",p_Back.ftype, p_Back.seqnum, p_Back.acknum, p_Back.checksum, p_Back.payload);
            tolayer1(1, p_Back);
        }
        else
        {
            tolayer3(1,packet.payload);
            printf("   Received successfully at B: seq: ftype: %d, %d, ack: %d, csum: %d, data: %s \n",packet.ftype, packet.seqnum, packet.acknum, packet.checksum, packet.payload);



                p_Back.ftype = 1;
                p_Back.seqnum = packet.seqnum;
                p_Back.acknum = back;
                strcpy(p_Back.payload, "ACK");
                p_Back.checksum = calCheckSum(p_Back);

                if(piggybacking == 0)
                {
                    printf("   Sending Ack: ftype: %d, seq: %d, ack: %d, csum: %d, data: %s \n",p_Back.ftype, p_Back.seqnum, p_Back.acknum, p_Back.checksum, p_Back.payload);
                    tolayer1(1, p_Back);
                }
                else
                {
                    outstandingACK_B = 1;
                    printf("Waiting for another message from B to A.\n");
                }
                back = 1 - back;
        }
    }
    else if (packet.ftype == 1 ) // received ack from sender A.
    {
        printf("      (Ack received from A)\n");
        if ( packet.acknum != bseq)
        {
            printf("   Received Ack: ftype: %d, seq: %d, ack: %d, csum: %d, data: %s\n", packet.ftype, packet.seqnum, packet.acknum, packet.checksum, packet.payload);
            printf("   Ignoring Ack, as it is ack for previous packet.\n");
            return;
        }
        else if ( packet.checksum != calCheckSum(packet) )
        {
            printf("   Received Ack(corrupted): ftype: %d, seq: %d, ack: %d, csum: %d, data: %s\n",packet.ftype, packet.seqnum, packet.acknum, packet.checksum, packet.payload);
        }
        else
        {
            printf("   Acknowledgment received Successfully. \n");
            printf("   Received Ack: ftype: %d, seq: %d, ack: %d, csum: %d, data: %s\n",packet.ftype, packet.seqnum, packet.acknum, packet.checksum, packet.payload);

            receivedMsgAck_B = 1; // full process ended
            bseq = 1 - bseq;

            stoptimer(1);
            printf("Whole process finished for the current message of B-->A. \n\n\n");
        }
    }
    else if (packet.ftype == 2) // received data + piigyback ack
    {
        printf("      (Data + Ack received from A)\n");
        if(packet.acknum == bseq )
        {
            printf("   Acknowledgment received Successfully. \n");
            receivedMsgAck_B = 1; // full process ended
            bseq = 1 - bseq;

            stoptimer(1);
            printf("Whole process finished for the current message of B-->A. \n\n\n");
        }

        p_Back.ftype = 1;
        if (packet.seqnum != back)
        {
            printf("   Received Duplicate Packet.     Dropped.\n");
            if(piggybacking == 1 && outstandingACK_B == 1)
            {
                printf("   Sending Ack: ftype: %d, seq: %d, ack: %d, csum: %d, data: %s \n",p_Back.ftype, p_Back.seqnum, p_Back.acknum, p_Back.checksum, p_Back.payload);
                tolayer1(1, p_Back);

                //back = 1 - back;
                outstandingACK_B = 0;
            }
        }
        else if (packet.checksum != calCheckSum(packet))
        {
            printf("   Packet Corrupted. \n   Sending ack(prev): ftype: %d, seq: %d, ack: %d, csum: %d, data:%s\n",p_Back.ftype, p_Back.seqnum, p_Back.acknum, p_Back.checksum, p_Back.payload);
            tolayer1(1, p_Back);
        }
        else
        {
            tolayer3(1,packet.payload);
            printf("   Received successfully at B: ftype: %d, seq: %d, ack: %d, csum: %d, data: %s \n",packet.ftype, packet.seqnum, packet.acknum, packet.checksum, packet.payload);

            p_Back.ftype = 1;
            p_Back.seqnum = packet.seqnum;
            p_Back.acknum = back;
            strcpy(p_Back.payload, "ACK");
            p_Back.checksum = calCheckSum(p_Back);

            if (piggybacking == 0 )
            {
                printf("   Sending Ack: ftype: %d, seq: %d, ack: %d, csum: %d, data: %s \n",p_Back.ftype, p_Back.seqnum, p_Back.acknum, p_Back.checksum, p_Back.payload);
                tolayer1(1, p_Back);
            }
            else
            {
                printf("Waiting for another message from B to A.\n ");
                outstandingACK_B = 1;
            }
            back = 1 - back;
        }

    }
}

/* called when B's timer goes off */
void B_timerinterrupt(void)
{
    if( receivedMsgAck_B == 0 )
    {
        printf("   B's Timer Interrupt is called. Resending message:\n");
        printf("   Resending: ftype: %d, seq: %d, ack: %d, csum: %d, data: %s \n", p_Bdata.ftype, p_Bdata.seqnum, p_Bdata.acknum, p_Bdata.checksum, p_Bdata.payload);
        tolayer1(1, p_Bdata);
        starttimer(1, 30);
    }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init(void)
{
    bseq = 0;
    back = 0;
}

/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
    - emulates the tranmission and delivery (possibly with bit-level corruption
        and packet loss) of packets across the layer 3/4 interface
    - handles the starting/stopping of a timer, and generates timer
        interrupts (resulting in calling students timer handler).
    - generates message to be sent (passed from later 5 to 4)

THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify
******************************************************************/

struct event
{
    float evtime;       /* event time */
    int evtype;         /* event type code */
    int eventity;       /* entity where event occurs */
    struct frm *pktptr; /* ptr to packet (if any) assoc w/ this event */
    struct event *prev;
    struct event *next;
};
struct event *evlist = NULL; /* the event list */

/* possible events: */
#define TIMER_INTERRUPT 0
#define FROM_layer3 1
#define FROM_layer1 2

#define OFF 0
#define ON 1
#define A 0
#define B 1

int TRACE = 1;     /* for my debugging */
int nsim = 0;      /* number of messages from 5 to 4 so far */
int nsimmax = 0;   /* number of msgs to generate, then stop */
float time = 0.000;
float lossprob;    /* probability that a packet is dropped  */
float corruptprob; /* probability that one bit is packet is flipped */
float lambda;      /* arrival rate of messages from layer 5 */
int ntolayer1;     /* number sent into layer 3 */
int nlost;         /* number lost in media */
int ncorrupt;      /* number corrupted by media*/

void init();
void generate_next_arrival(void);
void insertevent(struct event *p);

int main()
{
    struct event *eventptr;
    struct pkt msg2give;
    struct frm pkt2give;

    int i, j;
    char c;

    init();
    A_init();
    B_init();

    while (1)
    {
        eventptr = evlist; /* get next event to simulate */
        if (eventptr == NULL)
            goto terminate;

        evlist = evlist->next; /* remove this event from event list */
        if (evlist != NULL)
            evlist->prev = NULL;

        if (TRACE >= 2)
        {
            printf("\nEVENT time: %f,", eventptr->evtime);
            printf("  type: %d", eventptr->evtype);
            if (eventptr->evtype == 0)
                printf(", timerinterrupt  ");
            else if (eventptr->evtype == 1)
                printf(", fromlayer3 ");
            else
                printf(", fromlayer1 ");
            printf(" entity: %d\n", eventptr->eventity);
        }
        time = eventptr->evtime; /* update time to next event time */
        if (eventptr->evtype == FROM_layer3)
        {
            if (nsim < nsimmax)
            {
                if (nsim + 1 < nsimmax)
                    generate_next_arrival(); /* set up future arrival */
                /* fill in msg to give with string of same letter */
                j = nsim % 26;
                for (i = 0; i < 4; i++)
                    msg2give.data[i] = 97 + j;
                msg2give.data[3] = 0;
                if (TRACE > 2)
                {
                    printf("          MAINLOOP: data given to student: ");
                    for (i = 0; i < 4; i++)
                        printf("%c", msg2give.data[i]);
                    printf("\n");
                }
                nsim++;
                if (eventptr->eventity == A)
                    A_output(msg2give);
                else
                    B_output(msg2give);
            }
        }
        else if (eventptr->evtype == FROM_layer1)
        {
            pkt2give.ftype = eventptr->pktptr->ftype;
            pkt2give.seqnum = eventptr->pktptr->seqnum;
            pkt2give.acknum = eventptr->pktptr->acknum;
            pkt2give.checksum = eventptr->pktptr->checksum;
            for (i = 0; i < 4; i++)
                pkt2give.payload[i] = eventptr->pktptr->payload[i];
            if (eventptr->eventity == A) /* deliver packet by calling */
                A_input(pkt2give); /* appropriate entity */
            else
                B_input(pkt2give);
            free(eventptr->pktptr); /* free the memory for packet */
        }
        else if (eventptr->evtype == TIMER_INTERRUPT)
        {
            if (eventptr->eventity == A)
                A_timerinterrupt();
            else
                B_timerinterrupt();
        }
        else
        {
            printf("INTERNAL PANIC: unknown event type \n");
        }
        free(eventptr);
    }

terminate:
    printf(
        " Simulator terminated at time %f\n after sending %d msgs from layer3\n",
        time, nsim);
}

void init() /* initialize the simulator */
{
    int i;
    float sum, avg;
    float jimsrand();

    printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
    printf("Enter the number of messages to simulate: ");
    scanf("%d",&nsimmax);
    printf("Enter  packet loss probability [enter 0.0 for no loss]:");
    scanf("%f",&lossprob);
    printf("Enter packet corruption probability [0.0 for no corruption]:");
    scanf("%f",&corruptprob);
    printf("Enter average time between messages from sender's layer3 [ > 0.0]:");
    scanf("%f",&lambda);
    printf("Piggybacking:");
    scanf("%d", &piggybacking);
    printf("Enter TRACE:");
    scanf("%d",&TRACE);

    srand(9999); /* init random number generator */
    sum = 0.0;   /* test random number generator for students */
    for (i = 0; i < 1000; i++)
        sum = sum + jimsrand(); /* jimsrand() should be uniform in [0,1] */
    avg = sum / 1000.0;
    if (avg < 0.25 || avg > 0.75)
    {
        printf("It is likely that random number generation on your machine\n");
        printf("is different from what this emulator expects.  Please take\n");
        printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
        exit(1);
    }

    ntolayer1 = 0;
    nlost = 0;
    ncorrupt = 0;

    time = 0.0;              /* initialize time to 0.0 */
    generate_next_arrival(); /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand(void)
{
    double mmm = RAND_MAX;
    float x;                 /* individual students may need to change mmm */
    x = rand() / mmm;        /* x should be uniform in [0,1] */
    return (x);
}

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

void generate_next_arrival(void)
{
    double x, log(), ceil();
    struct event *evptr;
    float ttime;
    int tempint;

    if (TRACE > 2)
        printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");

    x = lambda * jimsrand() * 2; /* x is uniform on [0,2*lambda] */
    /* having mean of lambda        */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtime = time + x;
    evptr->evtype = FROM_layer3;
    if (BIDIRECTIONAL && (jimsrand() > 0.5))
        evptr->eventity = B;
    else
        evptr->eventity = A;
    insertevent(evptr);
}

void insertevent(struct event *p)
{
    struct event *q, *qold;

    if (TRACE > 2)
    {
        printf("            INSERTEVENT: time is %lf\n", time);
        printf("            INSERTEVENT: future time will be %lf\n", p->evtime);
    }
    q = evlist;      /* q points to header of list in which p struct inserted */
    if (q == NULL)   /* list is empty */
    {
        evlist = p;
        p->next = NULL;
        p->prev = NULL;
    }
    else
    {
        for (qold = q; q != NULL && p->evtime > q->evtime; q = q->next)
            qold = q;
        if (q == NULL)   /* end of list */
        {
            qold->next = p;
            p->prev = qold;
            p->next = NULL;
        }
        else if (q == evlist)     /* front of list */
        {
            p->next = evlist;
            p->prev = NULL;
            p->next->prev = p;
            evlist = p;
        }
        else     /* middle of list */
        {
            p->next = q;
            p->prev = q->prev;
            q->prev->next = p;
            q->prev = p;
        }
    }
}

void printevlist(void)
{
    struct event *q;
    int i;
    printf("--------------\nEvent List Follows:\n");
    for (q = evlist; q != NULL; q = q->next)
    {
        printf("Event time: %f, type: %d entity: %d\n", q->evtime, q->evtype,
               q->eventity);
    }
    printf("--------------\n");
}

/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
void stoptimer(int AorB /* A or B is trying to stop timer */)
{
    struct event *q, *qold;

    if (TRACE > 2)
        printf("          STOP TIMER: stopping timer at %f\n", time);
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB))
        {
            /* remove this event */
            if (q->next == NULL && q->prev == NULL)
                evlist = NULL;          /* remove first and only event on list */
            else if (q->next == NULL) /* end of list - there is one in front */
                q->prev->next = NULL;
            else if (q == evlist)   /* front of list - there must be event after */
            {
                q->next->prev = NULL;
                evlist = q->next;
            }
            else     /* middle of list */
            {
                q->next->prev = q->prev;
                q->prev->next = q->next;
            }
            free(q);
            return;
        }
    printf("Warning: unable to cancel your timer. It wasn't running.\n");
}

void starttimer(int AorB /* A or B is trying to start timer */, float increment)
{
    struct event *q;
    struct event *evptr;

    if (TRACE > 2)
        printf("          START TIMER: starting timer at %f\n", time);
    /* be nice: check to see if timer is already started, if so, then  warn */
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB))
        {
            printf("Warning: attempt to start a timer that is already started\n");
            return;
        }

    /* create future event for when timer goes off */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtime = time + increment;
    evptr->evtype = TIMER_INTERRUPT;
    evptr->eventity = AorB;
    insertevent(evptr);
}

/************************** TOlayer1 ***************/
void tolayer1(int AorB, struct frm packet)
{
    struct frm *mypktptr;
    struct event *evptr, *q;
    float lastime, x;
    int i;

    ntolayer1++;

    /* simulate losses: */
    if (jimsrand() < lossprob)
    {
        nlost++;
        if (TRACE > 0)
            printf("          TOlayer1: packet being lost\n");
        return;
    }

    /* make a copy of the packet student just gave me since he/she may decide */
    /* to do something with the packet after we return back to him/her */
    mypktptr = (struct frm *)malloc(sizeof(struct frm));
    mypktptr->ftype = packet.ftype;
    mypktptr->seqnum = packet.seqnum;
    mypktptr->acknum = packet.acknum;
    mypktptr->checksum = packet.checksum;
    for (i = 0; i < 4; i++)
        mypktptr->payload[i] = packet.payload[i];
    if (TRACE > 2)
    {
        printf("          TOlayer1: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
               mypktptr->acknum, mypktptr->checksum);
        for (i = 0; i < 4; i++)
            printf("%c", mypktptr->payload[i]);
        printf("\n");
    }

    /* create future event for arrival of packet at the other side */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtype = FROM_layer1;      /* packet will pop out from layer1 */
    evptr->eventity = (AorB + 1) % 2; /* event occurs at other entity */
    evptr->pktptr = mypktptr;         /* save ptr to my copy of packet */
    /* finally, compute the arrival time of packet at the other end.
       medium can not reorder, so make sure packet arrives between 1 and 10
       time units after the latest arrival time of packets
       currently in the medium on their way to the destination */
    lastime = time;
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == FROM_layer1 && q->eventity == evptr->eventity))
            lastime = q->evtime;
    evptr->evtime = lastime + 1 + 9 * jimsrand();

    /* simulate corruption: */
    if (jimsrand() < corruptprob)
    {
        ncorrupt++;
        if ((x = jimsrand()) < .75)
            mypktptr->payload[0] = 'Z'; /* corrupt payload */
        else if (x < .875)
            mypktptr->seqnum = 999999;
        else
            mypktptr->acknum = 999999;
        if (TRACE > 0)
            printf("          TOlayer1: packet being corrupted\n");
    }

    if (TRACE > 2)
        printf("          TOlayer1: scheduling arrival on other side\n");
    insertevent(evptr);
}

void tolayer3(int AorB, char datasent[4])
{
    int i;
    if (TRACE > 2)
    {
        printf("          TOlayer3: data received: ");
        for (i = 0; i < 4; i++)
            printf("%c", datasent[i]);
        printf("\n");
    }
}


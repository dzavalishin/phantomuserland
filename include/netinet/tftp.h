
/*

THE TFTP PROTOCOL (REVISION 2)
https://tools.ietf.org/html/rfc1350

  TFTP supports five types of packets, all of which have been mentioned
   above:

          opcode  operation
            1     Read request (RRQ)
            2     Write request (WRQ)
            3     Data (DATA)
            4     Acknowledgment (ACK)
            5     Error (ERROR)

   The TFTP header of a packet contains the  opcode  associated  with
   that packet.
*/

#define TFTP_PACKET_READ_RQ     1
#define TFTP_PACKET_WRITE_RQ    2
#define TFTP_PACKET_DATA        3
#define TFTP_PACKET_ACK         4
#define TFTP_PACKET_ERROR       5


#define TFTP_MAX_PACKET 512

struct tftp_t 
{
    unsigned short opcode;

    union {
        struct {
            unsigned short block;
        } ack;

        struct {
            unsigned short block;
            char download[TFTP_MAX_PACKET];
        } data;

		char rrq[TFTP_MAX_PACKET];

		struct {
			unsigned short errcode;
			char errmsg[TFTP_MAX_PACKET];
		} err;

		struct {
			char data[TFTP_MAX_PACKET+2];
		} oack;

    } u;
};

#ifndef RFID_QUEUE_H
#define RFID_QUEUE_H
#define RFID_Control   PDout(10)
typedef int Qsize_t;
typedef char Qdata_t;

void rfid_queue_push(Qdata_t);
int vGet_rfid_num(unsigned long *);
void vClose_rfid();
void vOpen_rfid();
#endif

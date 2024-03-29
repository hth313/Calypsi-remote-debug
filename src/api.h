#ifndef _API_H_
#define _API_H_

// Target API
extern void initializeTarget(void);
extern char getDebugChar(void);
extern void putDebugChar(char);
extern void continueExecution(void*);
extern void enableSerialInterrupt(void);
extern void disableSerialInterrupt(void);

#endif // _API_H_

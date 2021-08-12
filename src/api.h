#ifndef _API_H_
#define _API_H_

// Target API
extern void initialize(void);
extern char getDebugChar(void);
extern void putDebugChar(char);
extern void continueExecution(void*);
extern void enableCtrlC(void);

#endif // _API_H_

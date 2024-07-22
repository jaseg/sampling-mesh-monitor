#ifndef __IOMACROS_H__
#define __IOMACROS_H__

#define IN(pin) (0)
#define OUT(pin) (1<<(2*(pin)))
#define AF(pin) (2<<(2*(pin)))
#define ANALOG(pin) (3<<(2*(pin)))

#define CLEAR(pin) (~(3<<(2*(pin))))

#define AFRL(pin, val) ((val) << ((pin)*4))
#define AFRH(pin, val) ((val) << (((pin)-8)*4))

#endif __IOMACROS_H__

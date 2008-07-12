
#define GETRAW(name)      (name)
#define GETRAWSIZE(name)  ((int)name##_size)
#define GETRAWEND(name)  ((int)name##_end)

typedef uchar		uint8;
typedef ushort	uint16;
typedef ulong	uint32;
typedef uvlong	uint64;

typedef char		int8;
typedef short		int16;
typedef long		int32;
typedef vlong		int64;

typedef float		float32;
typedef double		float64;

typedef volatile uchar	vuint8;
typedef volatile ushort	vuint16;
typedef volatile ulong	vuint32;
typedef volatile uvlong	vuint64;

typedef volatile char		vint8;
typedef volatile short	vint16;
typedef volatile long	vint32;
typedef volatile vlong	vint64;

typedef volatile float32        vfloat32;
typedef volatile float64        vfloat64;

typedef uchar		byte;

typedef uchar		u8;
typedef ushort	u16;
typedef ulong	u32;
typedef uvlong	u64;

typedef char		s8;
typedef short		s16;
typedef long		s32;
typedef vlong		s64;

typedef volatile u8          vu8;
typedef volatile u16         vu16;
typedef volatile u32         vu32;
typedef volatile u64         vu64;

typedef volatile s8           vs8;
typedef volatile s16          vs16;
typedef volatile s32          vs32;
typedef volatile s64          vs64;

typedef struct touchPosition touchPosition;
struct touchPosition {
	int16	x;
	int16	y;
	int16	px;
	int16	py;
	int16	z1;
	int16	z2;
};

typedef enum { false, true } bool;

// Handy function pointer typedefs
typedef void ( * IntFn)(void);
typedef void (* VoidFunctionPointer)(void);
typedef void (* fp)(void);


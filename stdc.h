#ifndef STDC_H
#define STDC_H

#if defined(__STDC__)
# define __P(x)		x
#else
# define __P(x)
# define const
#endif

#endif /* STDC_H */

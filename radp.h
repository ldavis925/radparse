#ifndef RADP_H
#define RADP_H

#ifdef __cplusplus
extern "C" {
#endif

#define	DOT(d)		((d)[0] == '.' && (d)[1] == '\0')
#define DOTDOT(d)	((d)[0] == '.' && (d)[1] == '.' && (d)[2] == '\0')
#define DOTFILE(d)	(DOT(d) || DOTDOT(d))

#define CEIL(x,y)	((x) > (y) ? (y) : (x))

#ifdef __cplusplus
}
#endif

#endif /* RADP_H */

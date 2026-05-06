#ifndef SGB_BORDER_H
#define SGB_BORDER_H

#include <stdint.h>

/* Upload our procedural SGB border. No-op if not running on SGB. Display
 * must already be ON when this is called. */
void sgb_border_install(void);

#endif

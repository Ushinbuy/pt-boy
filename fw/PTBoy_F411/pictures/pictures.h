#ifndef PICTURES_H_
#define PICTURES_H_

#include "main.h"

void picturesShowLogoAnimation();

void picturesSetupSlideShow();
void picturesExecute(PushedButton handle);

int picturesIsNewPicureUnlock(int oldScores, int newScores);

#endif /* PICTURES_H_ */

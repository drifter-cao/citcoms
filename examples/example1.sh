#!/bin/sh
#
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#
# <LicenseText>
#
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#

../tests/citcomsregional.sh \
\
--steps=71 \
\
--controller.monitoringFrequency=10 \
\
--staging.nodes=4 \
--staging.nodegen="n%03d" \
--staging.nodelist=[101-102,101-102] \
\
--solver.datafile=/scratch/username/example1 \
\
--solver.mesher.nprocx=2 \
--solver.mesher.nprocy=2 \
--solver.mesher.nodex=17 \
--solver.mesher.nodey=17 \
--solver.mesher.nodez=9 \


# version
# $Id: example1.sh,v 1.3 2004/06/29 17:25:11 tan2 Exp $

# End of file
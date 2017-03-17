Logger for SBIG all-sky 
====================

Store raw FITS images from SBIG FITS dumps adding to headers cloud sensor
information.

Files stored in subdirectories as ${PWD}/YYYY/MM/DD/hhmmss.fits

The original FITS file should be rewrited by SBIG daemon, this daemon watch
for changes using inotify and as only file changed, store it appending boltwood
data.

/**
 Jouk Jansen <joukj@alpha.chem.uva.nl> contributed this
 which he found at http://axp616.gsi.de:8080/www/vms/mzsw.html

 quick hack for sounds in xlockmore on VMS
 with a the above AUDIO package slightly modified
**/

/** vms_play.c **/

#include <stdio.h>
#include <file.h>
#include <unixio.h>
#include <iodef.h>
#include "amd.h"

void vms_play(char *filename)
{

	int i,j, status;
	char buffer[2048];
	int volume = 65;		/* volume is default to 65% */
	int speaker = SO_INTERNAL /*SO_EXTERNAL*/;	/* use the internal speaker(s) */
	int fp;

	/*
	 *	Get user arguments
	 */
/*	i = 1;
	while (i < argc) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
				case 'v':	*//* get the volume *//*
						i++;
						volume = atol(argv[i++]);
						break;

				case 'i':	*//* use the internal speaker *//*
						i++;
						speaker = SO_INTERNAL;
						break;

				case 'h':	*//* display usage *//*
						Usage();
						exit(1);

				default:
						fprintf(stderr, "%%PLAY-F-INVPARM, invalid parameter %s\n", argv[i]);
						Usage();
						exit(1);
			}
		} else {
			filename = argv[i++];
		}
	}*/
	/*
	 *	Initialize access to AMD
	 */
	status = AmdInitialize("SO:", volume);
	/*
 	 *	Select which speaker
	 */
	AmdSelect(speaker);
	/*
	 *	Open the file
	 */
	fp = open(filename,O_RDONLY,0777);
	/*
	 *	Read through it
	 */
        if (!(fp==-1)) {
	  i = read(fp, buffer, 2048);
	  while(i) {
		status = AmdWrite(buffer, i);
		if (!(status&1)) exit(status);
		i = read(fp, buffer, 1024);
	  }
	}
	/*
	 *	Close the file
	 */
	close(fp);
	/*
	 *	Exit
	 */
}

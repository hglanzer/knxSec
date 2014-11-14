/*
    EIB Demo program - text busmonitor
    Copyright (C) 2005-2011 Martin Koegler <mkoegler@auto.tuwien.ac.at>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include "common.h"

int
main (int ac, char *ag[])
{
  uchar buf[255];
  int len;
  EIBConnection *con;
  if (ac != 2)
	return -1;
  con = EIBSocketURL (ag[1]);
  if (!con)
	return -1;

  if (EIBOpenBusmonitorText (con) == -1)
	return -1;


  printf("OK, waiting for packages\n");

  while (1)
    {
      len = EIBGetBusmonitorPacket (con, sizeof (buf), buf);
      if (len == -1)
	return -1;
      buf[len] = 0;
      printf ("len = %d, %s\n", len, buf);
      fflush (stdout);
    }

  EIBClose (con);
  return 0;
}

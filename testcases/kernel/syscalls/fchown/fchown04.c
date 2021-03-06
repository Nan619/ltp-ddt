/*
 *
 *   Copyright (c) International Business Machines  Corp., 2001
 *
 *   This program is free software;  you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY;  without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 *   the GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program;  if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/*
 * Test Name: fchown04
 *
 * Test Description:
 *   Verify that,
 *   1) fchown(2) returns -1 and sets errno to EPERM if the effective user id
 *	of process does not match the owner of the file and the process is
 *	not super user.
 *   2) fchown(2) returns -1 and sets errno to EBADF if the file descriptor
 *	of the specified file is not valid.
 *
 * Expected Result:
 *  fchown() should fail with return value -1 and set expected errno.
 *
 * Algorithm:
 *  Setup:
 *   Setup signal handling.
 *   Create temporary directory.
 *   Pause for SIGUSR1 if option specified.
 *
 *  Test:
 *   Loop if the proper options are given.
 *   Execute system call
 *   Check return code, if system call failed (return=-1)
 *	if errno set == expected errno
 *		Issue sys call fails with expected return value and errno.
 *	Otherwise,
 *		Issue sys call fails with unexpected errno.
 *   Otherwise,
 *	Issue sys call returns unexpected value.
 *
 *  Cleanup:
 *   Print errno log and/or timing stats if options given
 *   Delete the temporary directory(s)/file(s) created.
 *
 * Usage:  <for command-line>
 *  fchown04 [-c n] [-e] [-i n] [-I x] [-P x] [-t]
 *     where,  -c n : Run n copies concurrently.
 *             -e   : Turn on errno logging.
 *	       -i n : Execute test n times.
 *	       -I x : Execute test for x seconds.
 *	       -P x : Pause for x seconds between iterations.
 *	       -t   : Turn on syscall timing.
 *
 * HISTORY
 *	07/2001 Ported by Wayne Boyer
 *
 * RESTRICTIONS:
 *  This test should be executed by 'non-super-user' only.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <grp.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "test.h"
#include "usctest.h"
#include "safe_macros.h"

#define MODE_RWX	(S_IRWXU|S_IRWXG|S_IRWXO)
#define FILE_MODE	(S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)
#define TEST_FILE1	"tfile_1"
#define TEST_FILE2	"tfile_2"

void setup1();			/* setup function to test chmod for EPERM */
void setup2();			/* setup function to test chmod for EBADF */

int fd1;			/* File descriptor for testfile1 */
int fd2;			/* File descriptor for testfile2 */

struct test_case_t {		/* test case struct. to hold ref. test cond's */
	int fd;
	int exp_errno;
	void (*setupfunc) ();
} test_cases[] = {
	{
	1, EPERM, setup1}, {
2, EBADF, setup2},};

char test_home[PATH_MAX];	/* variable to hold TESTHOME env */
char *TCID = "fchown04";	/* Test program identifier.    */
int TST_TOTAL = 2;		/* Total number of test cases. */
int exp_enos[] = { EPERM, EBADF, 0 };

char bin_uid[] = "bin";
struct passwd *ltpuser;

void setup();			/* Main setup function for the tests */
void cleanup();			/* cleanup function for the test */

int main(int ac, char **av)
{
	int lc;
	char *msg;
	int fd;			/* test file descriptor */
	int i;
	uid_t user_id;		/* Effective user id of a test process */
	gid_t group_id;		/* Effective group id of a test process */

	if ((msg = parse_opts(ac, av, NULL, NULL)) != NULL)
		tst_brkm(TBROK, NULL, "OPTION PARSING ERROR - %s", msg);

	setup();

	TEST_EXP_ENOS(exp_enos);

	user_id = geteuid();
	group_id = getegid();

	for (lc = 0; TEST_LOOPING(lc); lc++) {

		Tst_count = 0;

		for (i = 0; i < TST_TOTAL; i++) {

			fd = test_cases[i].fd;

			if (fd == 1)
				fd = fd1;
			else
				fd = fd2;

			TEST(fchown(fd, user_id, group_id));

			if (TEST_RETURN == -1) {
				if (TEST_ERRNO == test_cases[i].exp_errno)
					tst_resm(TPASS | TTERRNO,
						 "fchown failed as expected");
				else
					tst_resm(TFAIL | TTERRNO,
						 "fchown failed unexpectedly; "
						 "expected %d - %s",
						 test_cases[i].exp_errno,
						 strerror(test_cases[i].
							  exp_errno));
			} else
				tst_resm(TFAIL, "fchown passed unexpectedly");
		}

	}

	cleanup();

	tst_exit();
}

void setup()
{
	int i;

	TEST_PAUSE;

	tst_require_root(NULL);

	ltpuser = SAFE_GETPWNAM(NULL, bin_uid);

	tst_tmpdir();

	tst_sig(FORK, DEF_HANDLER, cleanup);

	initgroups("root", getgid());

	SAFE_SETEGID(NULL, ltpuser->pw_uid);
	SAFE_SETEUID(NULL, ltpuser->pw_gid);

	tst_tmpdir();

	for (i = 0; i < TST_TOTAL; i++)
		if (test_cases[i].setupfunc != NULL)
			test_cases[i].setupfunc();
}

void setup1()
{
	int old_uid;
	struct passwd *nobody;

	fd1 = SAFE_OPEN(cleanup, TEST_FILE1, O_RDWR | O_CREAT, 0666);

	old_uid = geteuid();

	SAFE_SETEUID(cleanup, 0);

	nobody = SAFE_GETPWNAM(cleanup, "nobody");

	if (fchown(fd1, nobody->pw_uid, nobody->pw_gid) < 0)
		tst_brkm(TBROK | TERRNO, cleanup, "fchown failed");

	SAFE_SETEUID(cleanup, old_uid);
}

void setup2()
{
	fd2 = SAFE_OPEN(cleanup, TEST_FILE2, O_RDWR | O_CREAT, 0666);
	SAFE_CLOSE(cleanup, fd2);
}

void cleanup()
{
	TEST_CLEANUP;

	SAFE_CLOSE(NULL, fd1);

	tst_rmdir();

}

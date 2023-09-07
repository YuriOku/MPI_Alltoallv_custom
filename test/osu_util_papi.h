
/*                          COPYRIGHT
*
* Copyright (c) 2001-2023, The Ohio State University. All rights
* reserved.
*
* The OMB (OSU Micro Benchmarks) software package is developed by the team
* members of The Ohio State University's Network-Based Computing Laboratory
* (NBCL), headed by Professor Dhabaleswar K. (DK) Panda.
*
* Contact:
* Prof. Dhabaleswar K. (DK) Panda
* Dept. of Computer Science and Engineering
* The Ohio State University
* 2015 Neil Avenue
* Columbus, OH - 43210-1277
* Tel: (614)-292-5199; Fax: (614)-292-2911
* E-mail:panda@cse.ohio-state.edu
*
* This program is available under BSD licensing.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*
* (1) Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
*
* (2) Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
*
* (3) Neither the name of The Ohio State University nor the names of
* their contributors may be used to endorse or promote products derived
* from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
* OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
 * PAPI uses -1 as a nonexistent hardware event placeholder
 * https://icl.utk.edu/papi/docs/df/d34/group__consts.html
 */
#ifdef _ENABLE_PAPI_
#define OMB_PAPI_NULL PAPI_NULL
#else
#define OMB_PAPI_NULL -1
#endif

#define OMB_PAPI_FILE_PATH_MAX_LENGTH OMB_FILE_PATH_MAX_LENGTH
#define OMB_PAPI_NUMBER_OF_EVENTS     100

void omb_papi_init(int *papi_eventset);
void omb_papi_start(int *papi_eventset);
void omb_papi_stop_and_print(int *papi_eventset, int size);
void omb_papi_free(int *papi_eventset);
void omb_papi_parse_event_options(char *opt_arr);

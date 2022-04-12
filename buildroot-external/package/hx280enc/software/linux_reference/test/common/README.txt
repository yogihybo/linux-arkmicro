*******************************************************************************
*******************************************************************************
*********** This is a step by step guide to 8290 encoder testing!  ************
*******************************************************************************
*******************************************************************************

1. Checking out, configuring and compiling SW.

1.0 Access to lab testing file server

    - You should have access to hantrodom4:/export
    - This is where all the testing takes place
    - cd to your own working directory export/work/user/

1.1 Checking SW out from version control.

    - 8290 project git repository is at:
        /afs/hantro.com/projects/8290/git/8290_encoder

    - First you have to clone the working tree from the git main repository:

    > git clone -n /afs/hantro.com/projects/8290/git/8290_encoder

    - You can see all the tags in the main tree with command

    > cd 8290_encoder
    > git tag

    - Then you need to check out the correct tag and make a branch of it

    > git checkout -b branch_x_x sys8290_x_x

    - Normally you get Software versions from tag mails you get
        from SW guys (SaPi). Tags can also be found from
        PWA at 8290 project site

1.2 Configuring and compiling SW

    - Go to 8290_encoder/software/linux_reference/test/common and run setup:

    > ./setup_lab_test.sh

    - This creates 8290_encoder/testdir and copies all the needed scripts there
    - Go to the masterscripts folder

    > cd ../../../../testdir/masterscripts

    - Open commonconfig.sh and edit the following lines:

        ->export swtag="enc8290_x_x"        # This is the tag with which you checked out the software from version control

        ->export hwtag="enc8290_x_x"        # This is the HW version that you are testing, you usually get it from tag mails, otherwise ask Ari Hautala.

        ->export systag="enc8290_x_x"       # This is the system model which corresponds to the software and hardware tags.

        ->export testdeviceip=45            # Set IP for test device, check the following lines that the
                                            # configuration is matching the board in use.
                                            #    4*:  VERSATILE (AHB-bus)
                                            #    7*:  EB (Emulation Board, AXI-bus)
                                            #    8*:  AXIVERSATILE (AXI-bus)

        ->REMOVE_HANTRO_USER_DATA="y"      #Use y for testing releases for Sony. Otherwise use n.

        ->COMPILER_SETTINGS=""             #Board specific value, this can be seen from board status monitor:
                                           #http://192.168.30.104/monitor/index.php


        ->DWL_IMPLEMENTATION="POLLING"     #Then you have to decide do you use IRQ or POLLING mode. IRQ's do not
                                           #work with all the boards yet, but normally they are recommended.
                                           #AXI versatile boards do not support IRQs yet so with
                                           #those you have to use polling mode.

        ->INTERNAL_TEST="y"         #With this flag you can either enable or disable internal
                                    #test cases from tests. Should be n with customer
                                    #releases, otherwise y

        ->MAX_WIDTH=""              #You can see from the hardware tag, which resolution
        ->MAX_HEIGHT=""             #this configuration supports.

        ->csv_path=""              #The directory where test reports are generated.

    - Run script ./set.sh in masterscript directory.

    - This script cleans the test directory, changes all the necessary software
        parameters automatically, compiles the software and test benches,
        and copies all the test scripts and binaries
        to the test directory.



2 Laboratory testing.

    - First you have to log in to test board from xterm.

    > telnet 192.168.30.xx
    > username -> root

    - Verify that the tag corresponds to the
        one in RTL, this can be checked from the hw base address when logged on to
        the board:
           > dm2 0xC0000000 (Versatile board),
                 0xC4000000 (Axi Versatile board) or
                 0x84000000 (Emulation Board)

    - Go to the /export/work/user/8290_encoder/testdir/masterscripts and run script

    > ./runall.sh

    - This script starts by loading the memalloc and driver modules to the board.

        - If something goes wrong probable reason is wrong kernel,
            wrong base address or wrong memory start address.
            Return value -1 normally means wrong kernel
        - You can check the board's kernel from http://192.168.30.104/monitor/
            so that it is the same you defined for the sw.

    - Then it runs a small test set "smoketest" to ensure that the test
        environment is set up correctly.

    - Open a new terminal, cd to your export working directory testdir/masterscripts

    - Run the script

    > ./checkall.sh

        to check the results. This must be running at the same time as runall.sh
        because the testing and checking are done parallel.

    - If everything goes ok, just wait for the tests to finish,
        this usually lasts until the next day.

    - If something goes wrong, check the file commonconfig.sh and start again
        from running set.sh.

    - Check test report files: integrationreport_format_tag_time.csv
        which are copied to /afs/hantro.com/projects/8290/integration/test_reports

    - Optionally you can also run the movie test lasting hours:
    > ./long_run_decode.sh on PC to decode a movie and then
    > ./long_run_encode.sh on board to encode it

    - Update tag comment in Project Web Access > 8290 > Tags:
        summary of tag testing, number of failed cases and reasons



High Fidelity Security Mesh Monitoring using Low-Cost, Embedded Time Domain Reflectometry
=========================================================================================

This repository contains all sources for our paper "High Fidelity Security Mesh Monitoring using Low-Cost, Embedded Time
Domain Reflectometry" to be published in CHES 2026/1 (`preprint on eprint <https://eprint.iacr.org/2025/1962>`__). The
canonical hosting location for this repo is `https://git.jaseg.de/sampling-mesh-monitor.git`__. For comments or
questions, please reach out to Jan via `email <mailto:research@jaseg.de>`__ or `on mastodon
<https://chaos.social/@jaseg>`__.

Paper abstract
--------------

    Security Meshes are patterns of sensing traces covering an area that are used in Hardware Security Modules (HSMs)
    and other systems to detect attempts to physically intrude into the device's protective shell. State-of-the-art
    solutions manufacture meshes in bespoke processes from carefully chosen materials, which is expensive and makes
    replication challenging. Additionally, state-of-the-art monitoring circuits sacrifice either monitoring precision or
    cost efficiency. In this paper, we present an embeddable security mesh monitoring circuit constructed from low-cost,
    standard components that utilizes Time Domain Reflectometry (TDR) to create a unique fingerprint of a mesh. Our
    approach is both low-cost and precise, and enables the use of inexpensive standard Printed Circuit Boards (PCBs) as
    security mesh material. We demonstrate a working prototype of our TDR circuit costing less than \price{10}{\euro} in
    components that achieves both time resolution and rise time better than \qty{200}{\pico\second}---a $25\times$
    improvement over previous work. We demonstrate a simple classifier that detects several types of advanced attacks
    such as probing using an oscilloscope probe or micro-soldering attacks with no false negatives.

Repo structure
--------------

``main_pcb``
    KiCad design files, exports and manufacturing files for the main circuit board. This board contains the measurement
    circuit, and has a connector for test coupons to be inserted. Data is read out throught the SWD interface using an
    SWD adapter. The board is a 6-layer

``test_coupons``
    KiCad design files, exports and manufacturing files for the test coupon PCBs. The coupons are four-layer boards.
    Each coupon contains four test meshes, two on each side, that can be connected through four SFP form factor board
    edge connectors at the coupon's corners.

``firmware``
    Source code for the firmware as well as tooling for running the experiments. The firmware is written in C, and
    you'll need a standard ARM GCC/binutils toolchain to build it. The experiment tool is a python script that connects
    to the board through GDB and exposes a local web server for controlling the board, monitoring the measurements in
    real time, and capturing data.


``analysis``
    All raw data, as well as the Jupyter notebooks used for analysis and plotting. By running these notebooks, you can
    re-run the analysis, and re-export all plots in the paper.

``paper``
    The source of the paper, as well as exports of all plots.

Opening the PCB designs
-----------------------

This repo includes both the raw CAD files of the PCBs, as well as exported manufacturing data for easier viewing if you
don't have KiCad installed. The schematics are included as PDF exports.

The PCB designs were prepared in the open source KiCad_ EDA tool. Note that for some files you might need to download a
KiCad nightly build depending on how new your KiCad version is.

PCB design folder structure
...........................

``foobar.{kicad_pro,kicad_sch,kicad_pcb,etc.}``
    The KiCad source files of the PCB. Open the ``.kicad_pro`` file in KiCad, then you can open the schematics and board
    layout from there.

``fab/gerbers.zip``
    Gerber exports of the board layouts. You can open these in any Gerber viewer such as gerbv_. These files are
    essentially vector graphics exports of the board layout. They show what copper, silkscreen etc. you see on the
    board, but they don't include any semantic information such as component values. You can send this zip directly to a
    PCB manufacturer and they will be able to manufacture these boards for you. The order parameters are included in the
    gerber files on the ``User.Comments`` layer.

``fab/{bom.csv,pos.csv}``
    Bill of materials and component placement coordinate files for board manufacturing. If you want a contract
    manufacturer to solder these boards for you, you need to give them these two files in addition to the gerbers. The
    BOM tells them what exact parts and how many of them you need, the position file tells them where to put them on the
    board. We had JLCPCB make the PCBs, and we also had them solder most of the parts. Some parts we soldered after the
    fact by hand because JLC didn't have them in stock, and getting them to order and solder them for us would have been
    pretty expensive.

    If you want to solder the main PCB yourself, you need some prior experience in fine SMD work and a decently setup
    electronics lab with a microscope, a hot air reworks station, etc.

    Note: **The PCB revision here does not have a functional driver IC!** For our lab experiments we microsoldered
    the four different driver ICs by hand to avoid having to spin multiple copies of this expensive PCB. For some basic
    measurements, you can just omit the driver IC included in the design and bridge it. This will drive the mesh
    directly from the ``74LVC2G157`` gates, which is enough for some basic measurements.

``schematics.pdf``
    PDF export of the schematic of the board.

Firmware folder structure
-------------------------

``src/main.c``
    The source code of the mainboard firmware. This firmware captures data into internal buffers, which are read out
    from the host through SWD.

``openocd.cfg``
    OpenOCD script for connecting to the debug adapter. We used cheap STLinkV2 clones from aliexpress. If you use a
    different debug adapter, you will have to modify this file accordingly to tell OpenOCD how to connect to the board.
    The port numbers given here and in the ``.gdbinit`` must match up.

``.gdbinit``
    GDB script running the data extraction. This script will continuously dump the measurement data into ``/tmp`` every
    time a measurement series has been completed. Note that this file will be hidden on unixes. Also note that you will
    have to explicitly allowlist this file's absolute path on your computer in your ``$HOME/.gdbinit`` the first time
    you run it because gdb refuses to run unknown ``.gdbinit`` files for security.

``adc_serve.py``
    Real-time monitoring and data capture tool. Before running this, first make sure you have gdb attached to the board
    using the ``.gdbinit``. The tool communicates with the gdb script through files in ``/tmp``. It exposes a web
    interface that displays the captured data in real time, allows you to change the board's measurement settings, and
    allows you to export measurement series.

``make_barcodes.py``
    This is a helper script we used to create barcode labels for all ~120 test specimens to simplify the measurement
    workflow. We had a barcode reader hooked up to the computer running the measurements, so you could just scan a
    specimen's barcode to capture a data series registered to that board in the web interface.
    
other files:
    Build files for the firmware. These are mostly generic to this type of microcontroller and are included here so the
    firmware source is self-contained.

.. _KiCad: https://www.kicad.org/
.. _gebrv: https://gerbv.github.io/

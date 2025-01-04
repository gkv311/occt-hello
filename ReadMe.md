Hello OpenCASCADE Technology!
=============================

[![Status](https://github.com/gkv311/occt-hello/actions/workflows/build_ubuntu_24.04.yml/badge.svg?branch=master)](https://github.com/gkv311/occt-hello/actions?query=branch%3Amaster)

This repository contains Hello-World samples accompanying articles devoted to learning [Open CASCADE Technology](https://dev.opencascade.org) (also abbreviated as OCCT) 3D Viewer API.
Samples require OCCT 7.6 or later for building.

## AIS Hello

Project within [`occt-ais-hello`](occt-ais-hello/) subfolder shows basic steps of OCCT 3D viewer on Windows, Linux and macOS platforms
based on [Tutorial](https://unlimited3d.wordpress.com/2021/03/27/occt-minimal-viewer-setup/).

![Hello World screenshot](/images/occt-ais-hello.png)

## AIS Object

Project within [`occt-ais-object`](occt-ais-object/) subfolder demonstrates definition of `AIS_InteractiveObject` subclass
based on [Tutorial](https://unlimited3d.wordpress.com/2021/11/16/ais-object-computing-presentation/).

![AIS Object screenshot](/images/occt-ais-object.gif)

## AIS Offscreen Dump

Project within [`occt-ais-offscreen`](occt-ais-offscreen/) subfolder demonstrates OCCT 3D viewer setup for offscreen image dump
based on [Tutorial](https://unlimited3d.wordpress.com/2022/01/30/offscreen-occt-viewer/).

![Offscreen screenshot](/images/occt-ais-offscreen.png)

## Draw Plugin

Project within [`occt-draw-plugin`](occt-draw-plugin/) subfolder demonstrates Draw Harness plugin sample.

## XCAF Shape

Project within [`occt-xcaf-shape`](occt-xcaf-shape/) subfolder demonstrates reading of STEP file into XCAF document and displaying it via `XCAFPrs_AISObject` in 3D Viewer.

![AIS Object screenshot](/images/occt-xcaf-shape.png)

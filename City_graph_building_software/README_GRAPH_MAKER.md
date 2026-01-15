# Graph Maker - City Network Builder

## Overview

**Graph Maker** is a custom-built, beginner-friendly web-based tool designed to create and manage comprehensive city geographic data. It enables users to visually build city networks by defining node coordinates, calculating street widths, and generating high-quality datasets containing thousands of locations across cities, colonies, highways, houses, hospitals, and other geographic points of interest.

## Purpose & Importance

### Why This Tool Was Created

The development of Graph Maker was driven by critical gaps in the existing software ecosystem:

- **Limited Availability**: No existing tools adequately fulfill the specific requirements for building custom city geographic databases
- **User-Friendliness Gap**: Available solutions often have steep learning curves unsuitable for quick data entry and visualization
- **Output Mismatch**: Existing tools don't produce outputs in the format and structure required by our Ride Sharing System
- **Customization Needs**: Off-the-shelf solutions lack the flexibility to handle our unique data requirements

### Critical Functionality

Graph Maker bridges these gaps by providing:

1. **Node Coordinate Management**: Assign precise GPS/map coordinates to each geographic point
2. **Width Calculation**: Automatically compute and manage street/path widths for distance calculations
3. **Mass Data Generation**: Create comprehensive city data containing:
   - Thousands of geographic locations
   - City colonies and districts
   - Highway networks
   - Residential areas (houses)
   - Healthcare facilities (hospitals)
   - Commercial zones
   - Any custom geographic points

4. **Quality Data Production**: Generate clean, validated, structured data for seamless integration into the Ride Sharing System

## Key Features

✅ **Intuitive Interface**: Designed for users of all technical levels—no steep learning curve required

✅ **Visual Network Building**: Click-based interface to create and modify city graphs

✅ **Coordinate Assignment**: Easy-to-use tools for assigning and editing node coordinates

✅ **Width Configuration**: Built-in width calculation tools for accurate distance modeling

✅ **Bulk Data Creation**: Generate thousands of data points efficiently in minimal time

✅ **Data Export**: Output structured data compatible with the Ride Sharing System

✅ **Flexible Categories**: Support for multiple location types (residences, hospitals, highways, etc.)

## How It Works

1. **Open the Tool**: Load `graph maker.html` in any modern web browser
2. **Create Nodes**: Click to place geographic points on the canvas
3. **Assign Coordinates**: Set latitude, longitude, and relevant metadata for each node
4. **Configure Width**: Define street/path widths for accurate routing
5. **Build Network**: Connect nodes to form city pathways and routes
6. **Generate Data**: Export the complete city dataset in the required format

## Output Data

The tool generates structured data files including:
- **city-locations.csv**: Comprehensive list of all geographic coordinates and node information
- **paths.csv**: Network connectivity and path width information

These files directly feed into the Ride Sharing System's data pipeline.

## Integration

Graph Maker is an essential component of the **Ride Sharing System** project, providing the foundational geographic data required for:
- Route optimization
- Distance calculations
- Location mapping
- Service area definition
- Dispatch engine functionality

## Why This Custom Solution Matters

In the absence of suitable commercial or open-source alternatives, Graph Maker represents a **bespoke solution** tailored precisely to our needs. It eliminates:
- Trial-and-error with mismatched tools
- Data format conversion complexities
- Learning unnecessary features from bloated applications
- Dependency on third-party vendors for critical functionality

By creating this tool, we maintain **control over data quality**, **flexibility for future extensions**, and **seamless integration** with our Ride Sharing System architecture.

---

**Created for**: Ride Sharing System Project  
**Data Output Formats**: CSV  
**Browser Compatibility**: All modern browsers supporting HTML5 Canvas and JavaScript

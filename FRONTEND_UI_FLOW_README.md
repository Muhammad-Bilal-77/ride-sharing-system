# Frontend Flow & UI Documentation for RideSharingSystem

## Table of Contents
- [Overview](#overview)
- [Main Application Window](#main-application-window)
- [Rider Dashboard (RiderWindow)](#rider-dashboard-riderwindow)
  - [Layout & Navigation](#layout--navigation)
  - [Book Ride Page](#book-ride-page)
  - [Trip Status Panel](#trip-status-panel)
  - [Trip History Page](#trip-history-page)
  - [Restored Trip UI](#restored-trip-ui)
- [City Map View (CityMapView)](#city-map-view-citymapview)
- [UI Technologies & Styling](#ui-technologies--styling)
- [Resource Management](#resource-management)

---

## Overview
The RideSharingSystem frontend is built using Qt (C++), leveraging both code-based and .ui (XML) layouts. The UI is designed for clarity, modern aesthetics, and a smooth user experience for both riders and system operators.

---

## Main Application Window
- **File:** mainwindow.cpp / mainwindow.ui
- **Class:** MainWindow (inherits QMainWindow)
- **Purpose:** Entry point and main navigation for the application.
- **Key UI Elements:**
  - Title bar with app name
  - Central widget with vertical layout
  - Home page with:
    - Large title label
    - Buttons: Choose User, Rollback Manager, Analytics
    - Decorative image (loaded via ResourceManager)
  - QMenuBar and QStatusBar for standard app controls
- **Navigation:** Uses a QStackedWidget to switch between Home, Rider Dashboard, Rollback Manager, and Analytics pages.

---

## Rider Dashboard (RiderWindow)
- **File:** riderwindow.cpp / riderwindow.h
- **Class:** RiderWindow (inherits QWidget)
- **Purpose:** Main interface for riders to book rides, view trip status, and access history.

### Layout & Navigation
- **Header:**
  - Back button (returns to main menu)
  - Welcome label (shows rider name)
  - Map button (opens map view)
- **Sidebar:**
  - Book Ride button (shows booking page)
  - History button (shows trip history)
- **Content Area:**
  - QStackedWidget switches between Book Ride and History pages

### Book Ride Page
- **Location Display:**
  - Shows current location with icon and label
  - Map image with overlay for current location
  - Button to see location on map (hover effect)
- **Destination Selection:**
  - Input field with placeholder and map icon
  - Button to pick destination on map
- **Request Ride:**
  - Prominent button to request a ride
- **Trip Status Panel:**
  - Shows current trip status, driver info, and allows ride cancellation (if eligible)

### Trip Status Panel
- **Displays:**
  - Current trip state (active, no trip, etc.)
  - Driver status
  - Cancel Ride button (only during pickup phase)

### Trip History Page
- **Scrollable list of past trips:**
  - Each trip card shows trip ID, status, pickup/dropoff, driver, distance, fare, and timestamp
  - Status color-coded (green for completed, red for cancelled)
  - Empty state message if no history

### Restored Trip UI
- **Special card shown if a trip is restored after rollback:**
  - Details: pickup, dropoff, fare, distance
  - Buttons: Continue Trip, Cancel Trip
  - Prominent warning styling

---

## City Map View (CityMapView)
- **File:** citymapview.cpp / citymapview.h
- **Class:** CityMapView (inherits QGraphicsView)
- **Purpose:** Interactive map for visualizing city layout, locations, and for picking destinations.
- **Features:**
  - Custom graphics items for streets, locations, and labels
  - Hover effects for location markers
  - Selection mode for picking locations
  - User location pin
  - Zoom and pan support
  - Dynamic label visibility based on zoom

---

## UI Technologies & Styling
- **Qt Widgets:** All UI is built using Qt's QWidget-based system.
- **.ui Files:** mainwindow.ui defines the main window structure; most other UIs are built in C++ for dynamic control.
- **Custom Styling:**
  - Extensive use of setStyleSheet for modern gradients, rounded corners, and hover/active effects
  - Consistent color palette (greens for action, reds for warnings, white backgrounds)
  - Responsive layouts using QVBoxLayout, QHBoxLayout, QGridLayout
- **Icons & Images:** Loaded via ResourceManager for portability

---

## Resource Management
- **ResourceManager:** Handles loading of images and icons from portable paths, supporting both development and packaged builds.
- **Images Used:**
  - App logo, map marker icons, map images, etc.

---

## Extending the UI
- Add new pages by extending QStackedWidget in MainWindow or RiderWindow
- Use consistent styling and layout patterns for new widgets
- For new map features, extend CityMapView and its custom QGraphicsItems

---

## References
- See riderwindow.cpp for detailed UI construction and logic
- See citymapview.cpp for map rendering and interaction
- See mainwindow.cpp/.ui for main navigation and entry flow

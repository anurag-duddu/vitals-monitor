# UI/UX Requirements: Bedside Vitals Monitor

## Document Information

| Attribute | Value |
|-----------|-------|
| Document ID | PRD-003 |
| Version | 1.0 |
| Status | Draft |
| Parent Document | PRD-001 Product Overview |
| UI Framework | LVGL |

---

## 1. Introduction

### 1.1 Purpose

This document defines the user interface and user experience requirements for the bedside vitals monitor. It establishes design principles, screen hierarchy, layout specifications, interaction patterns, and accessibility requirements that ensure clinical usability and safety.

### 1.2 Design Philosophy

The interface design is guided by these core principles:

| Principle | Description | Rationale |
|-----------|-------------|-----------|
| **Clinical Efficiency** | Minimize time to complete tasks | Nurses have limited time per patient |
| **Glanceable Status** | Critical information visible at distance | Bedside visibility during rounds |
| **Error Prevention** | Prevent mistakes before they happen | Patient safety |
| **Alarm Prominence** | Alarms impossible to miss | Life-critical notification |
| **Consistent Patterns** | Same interactions throughout | Reduce learning curve |
| **Minimal Cognitive Load** | Don't make users think | High-stress environment |

### 1.3 Requirement Notation

| Prefix | Category |
|--------|----------|
| UX-GEN | General Principles |
| UX-SCR | Screen Specifications |
| UX-LAY | Layout Requirements |
| UX-NAV | Navigation |
| UX-INT | Interaction Patterns |
| UX-ACC | Accessibility |
| UX-VIS | Visual Design |

---

## 2. General Design Principles

### 2.1 Information Hierarchy

#### UX-GEN-001: Primary Information Visibility
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Critical vital signs shall be visible from 2 meters distance under normal ward lighting |

**Implementation:**
- Large numeric displays (minimum 25mm digit height for primary vitals)
- High contrast between text and background
- No clutter obscuring vital parameters

#### UX-GEN-002: Information Layering
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Information shall be organized in clear layers of importance |

| Layer | Content | Visual Treatment |
|-------|---------|------------------|
| Primary | Current vital values, active alarms | Largest, most prominent |
| Secondary | Waveforms, trends summary | Medium prominence |
| Tertiary | Patient info, time, system status | Smaller, peripheral |
| Background | Grid lines, labels, decorative | Subtle, non-competing |

#### UX-GEN-003: Screen Real Estate Allocation
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Screen space shall be allocated to maximize clinical utility |

**Allocation Guidelines:**
- Vital sign numerics: 40-50% of screen
- Waveforms: 25-35% of screen
- Status/navigation/chrome: 15-20% of screen
- Alarm bar: Always visible, fixed position

### 2.2 Consistency Standards

#### UX-GEN-010: Visual Consistency
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Visual elements shall be consistent throughout the application |

**Consistency Requirements:**
- Same colors for same meanings (see Color Palette)
- Same icons for same functions
- Same typography scale
- Same spacing rhythm
- Same interaction feedback

#### UX-GEN-011: Interaction Consistency
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Interaction patterns shall be consistent and predictable |

**Patterns:**
- Tap: Primary action, selection
- Long press: Secondary actions, context menu
- Swipe: Navigation between screens (if implemented)
- Pinch: Zoom (trend charts only)

#### UX-GEN-012: Terminology Consistency
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Clinical terminology shall be consistent with international standards |

**Standard Labels:**
| Parameter | Abbreviation | Full Name |
|-----------|--------------|-----------|
| Heart Rate | HR | Heart Rate |
| Oxygen Saturation | SpO2 | Peripheral Oxygen Saturation |
| Blood Pressure | NIBP | Non-Invasive Blood Pressure |
| Systolic | SYS | Systolic |
| Diastolic | DIA | Diastolic |
| Mean Arterial Pressure | MAP | Mean Arterial Pressure |
| Temperature | Temp | Temperature |
| Respiratory Rate | RR | Respiratory Rate |

---

## 3. Screen Architecture

### 3.1 Screen Hierarchy

#### UX-SCR-001: Screen Structure
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | The application shall be organized into distinct screens |

```
┌─────────────────────────────────────────────────────────────┐
│                     SCREEN HIERARCHY                        │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────┐                                            │
│  │   Login     │ ─── Entry point (if auth required)        │
│  └──────┬──────┘                                            │
│         │                                                   │
│  ┌──────▼──────┐                                            │
│  │ Main Vitals │ ─── Primary operating screen              │
│  └──────┬──────┘                                            │
│         │                                                   │
│    ┌────┴────┬────────────┬────────────┬──────────┐        │
│    │         │            │            │          │        │
│    ▼         ▼            ▼            ▼          ▼        │
│ ┌──────┐ ┌──────┐   ┌──────────┐ ┌─────────┐ ┌────────┐   │
│ │Trends│ │Alarms│   │ Patient  │ │Settings │ │ System │   │
│ └──────┘ └──────┘   └──────────┘ └─────────┘ └────────┘   │
│                                                             │
│  Overlays (appear on any screen):                          │
│  • Active alarm modal                                       │
│  • NIBP in progress                                         │
│  • Confirmation dialogs                                     │
│  • On-screen keyboard                                       │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### 3.2 Screen Specifications

#### UX-SCR-010: Login Screen
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Authentication screen for user login |

**Elements:**
- Hospital/device identifier
- PIN entry pad (numeric)
- Badge tap prompt
- Login button
- Emergency access option (audit logged)

**Behavior:**
- Auto-focus on PIN field
- Clear error feedback on invalid credentials
- Timeout on inactivity returns to login
- No patient data visible on login screen

#### UX-SCR-011: Main Vitals Screen
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Primary monitoring screen displaying all vital parameters |

**Layout Zones:**

```
┌─────────────────────────────────────────────────────────────┐
│ [Alarm Bar - Always Visible]                           [Time]│
├─────────────────────────────────────────────────────────────┤
│                                    │                        │
│   ┌─────────────────────────────┐  │  ┌──────────────────┐  │
│   │                             │  │  │       HR         │  │
│   │       ECG Waveform          │  │  │      ❤ 72        │  │
│   │                             │  │  │      bpm         │  │
│   └─────────────────────────────┘  │  └──────────────────┘  │
│                                    │                        │
│   ┌─────────────────────────────┐  │  ┌──────────────────┐  │
│   │                             │  │  │      SpO2        │  │
│   │     Pleth Waveform          │  │  │       98         │  │
│   │                             │  │  │        %         │  │
│   └─────────────────────────────┘  │  └──────────────────┘  │
│                                    │                        │
│                                    │  ┌──────────────────┐  │
│                                    │  │      NIBP        │  │
│                                    │  │   120/80 (93)    │  │
│                                    │  │      mmHg        │  │
│                                    │  └──────────────────┘  │
│                                    │                        │
│                                    │  ┌────────┐┌────────┐  │
│                                    │  │ Temp   ││  RR    │  │
│                                    │  │ 36.8°C ││ 16/min │  │
│                                    │  └────────┘└────────┘  │
├─────────────────────────────────────────────────────────────┤
│ [Patient: Room 302-A] [◉ Trends] [◉ Alarms] [◉ Menu]       │
└─────────────────────────────────────────────────────────────┘
```

**Required Elements:**
- Alarm bar (top, always visible)
- Current time display
- All 5 vital parameters with current values
- ECG waveform
- Pleth waveform
- Patient identifier
- Navigation controls
- NIBP start button (accessible without navigation)

#### UX-SCR-012: Trends Screen
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Historical trend visualization |

**Layout:**
- Time range selector (1h, 4h, 8h, 12h, 24h, 72h)
- Parameter selector (which vitals to display)
- Graphical trend area (main content)
- Alarm event markers on timeline
- Navigation controls (pan, zoom)
- Return to main vitals button

**Graphical Area Features:**
- Time on X-axis
- Parameter value on Y-axis
- Alarm threshold lines displayed
- Grid lines for reference
- Touch to inspect specific point

#### UX-SCR-013: Alarms Screen
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Alarm history and configuration |

**Tabs/Sections:**
- Active Alarms: Currently active, unresolved
- Alarm History: Past 72 hours, chronological
- Alarm Limits: Current threshold settings

**Alarm List Item:**
- Timestamp
- Parameter
- Alarm type (high/low/technical)
- Priority indicator
- Value at alarm time
- Resolution status

#### UX-SCR-014: Patient Screen
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Patient identification and session management |

**Sections:**
- Current Patient: Demographics, identifiers
- Admit New Patient: Manual entry, barcode, RFID, ABHA
- Discharge Patient: End session workflow
- Patient History: Previous sessions (if applicable)

#### UX-SCR-015: Settings Screen
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | Device configuration and preferences |

**Accessible to Nurses:**
- Display brightness
- Sound volume
- Alarm pause duration
- Patient-specific alarm limits

**Accessible to Administrators:**
- Network configuration
- EHR integration settings
- Default alarm limits
- System preferences
- User management

#### UX-SCR-016: System Screen
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | System information and diagnostics |

**Sections:**
- Device information (model, serial, software version)
- Network status
- Storage status
- Sensor status
- Audit log viewer (admin only)
- Software update (admin only)

---

## 4. Layout Requirements

### 4.1 Responsive Design

#### UX-LAY-001: Responsive Layout System
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | The UI shall adapt to different screen sizes and orientations |

**Approach:**
Since LVGL requires manual layout management, define layout variants:

| Screen Class | Resolution Range | Layout |
|--------------|------------------|--------|
| Small | 800x480 - 1024x600 | Compact, reduced waveform height |
| Medium | 1024x768 - 1280x800 | Standard layout |
| Large | 1920x1080+ | Expanded, additional detail |

**Orientation:**
- Landscape: Primary, optimized layout
- Portrait: Supported, vertical arrangement

#### UX-LAY-002: Grid System
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | Layout shall follow consistent grid system |

**Grid Specifications:**
- Base unit: 8px
- All spacing multiples of base unit
- Column-based layout for main content areas
- Consistent margins and padding

### 4.2 Component Layout

#### UX-LAY-010: Vital Parameter Box
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Standard layout for vital sign display |

```
┌─────────────────────────────┐
│  HR            [limit icons]│  ← Parameter label, alarm indicators
│                             │
│         72                  │  ← Large numeric value (primary)
│                             │
│        bpm                  │  ← Unit
│  [source] [quality]         │  ← Secondary info
└─────────────────────────────┘
```

**Sizing:**
- Large: HR, SpO2 (most critical, most viewed)
- Medium: NIBP
- Small: Temp, RR (or combined panel)

#### UX-LAY-011: Waveform Display Area
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Standard layout for waveform display |

```
┌─────────────────────────────────────────────────────────────┐
│ ECG  II  25mm/s  10mm/mV  [freeze]                         │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│ ═══════════════╤═══════════════════════════════════════    │
│                │                    ∧                       │
│ ───────────────┼────────────────────────────────────────   │
│                │                    │                       │
│ ═══════════════╧════════════════════╧══════════════════    │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

**Elements:**
- Label with lead, sweep speed, scale
- Control buttons (freeze, scale adjust)
- Waveform trace area with grid
- Clear boundaries

#### UX-LAY-012: Alarm Bar
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Fixed alarm notification area |

```
┌─────────────────────────────────────────────────────────────┐
│ ⚠ HIGH: HR > 150 bpm (156)  [ACK]  │  No other alarms  │🔇│
└─────────────────────────────────────────────────────────────┘
```

**Specifications:**
- Fixed position (top of screen)
- Always visible (not scrollable away)
- Priority color background
- Alarm message text
- Acknowledge button
- Alarm silence/pause indicator

#### UX-LAY-013: Navigation Bar
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Primary navigation control area |

```
┌─────────────────────────────────────────────────────────────┐
│ [Patient ID]        [Trends] [Alarms] [Patient] [Settings] │
└─────────────────────────────────────────────────────────────┘
```

**Specifications:**
- Fixed position (bottom of screen)
- Current patient identifier
- Navigation buttons to main screens
- Active screen indicator
- Consistent across all screens

---

## 5. Navigation Requirements

### 5.1 Navigation Patterns

#### UX-NAV-001: Primary Navigation
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Navigation between main screens |

**Pattern:**
- Tab-style navigation via bottom bar
- Single tap to switch screens
- Visual indicator of active screen
- Main Vitals always accessible (home)

**Navigation Flow:**
```
Main Vitals ←→ Trends
     ↕           ↕
Patient   ←→  Alarms
     ↕           ↕
Settings  ←→  System
```

#### UX-NAV-002: Return to Monitoring
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Quick return to main monitoring screen |

**Mechanisms:**
- Home button always visible
- Auto-return to Main Vitals after configurable timeout (e.g., 2 minutes)
- Alarm activation returns to Main Vitals (configurable)
- One-tap return from any screen depth

#### UX-NAV-003: Modal Interactions
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Modal dialog behavior |

**When to Use Modals:**
- Confirmations (discharge patient, change limits)
- Alarm acknowledgment (optional full-screen)
- Error messages
- Data entry (patient info)

**Modal Behavior:**
- Dim background
- Centered on screen
- Clear close/cancel action
- Vitals still visible if not full-screen
- Alarms can interrupt modal

### 5.2 Navigation Depth

#### UX-NAV-010: Shallow Navigation
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Minimize navigation depth |

**Requirement:**
- Maximum 2 levels from Main Vitals to any function
- Most common tasks: 1 tap from Main Vitals
- Settings hierarchically organized but searchable

#### UX-NAV-011: Breadcrumbs and Context
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | User shall always know their location |

**Indicators:**
- Active tab highlighted in navigation bar
- Screen title in header area
- Back button where applicable
- Breadcrumb for deep navigation (Settings sub-menus)

---

## 6. Interaction Requirements

### 6.1 Touch Interaction

#### UX-INT-001: Touch Target Size
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Touch targets shall be appropriately sized for clinical use |

**Specifications:**
- Minimum touch target: 44x44 px (10mm physical)
- Recommended touch target: 60x60 px
- Critical actions (alarm ack): 80x80 px minimum
- Adequate spacing between targets (minimum 8px gap)

#### UX-INT-002: Touch Feedback
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Touch interactions shall provide immediate feedback |

**Feedback Types:**
- Visual: Button state change (pressed state)
- Audible: Optional key click sound (configurable)
- Color: Brief highlight on touch

**Timing:**
- Visual feedback: < 100ms from touch
- Action completion: < 500ms for simple actions

#### UX-INT-003: Gesture Support
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | Standard gestures for navigation and manipulation |

| Gesture | Action | Context |
|---------|--------|---------|
| Single tap | Select, activate | All interactive elements |
| Long press | Secondary action, context menu | Limited use |
| Swipe horizontal | Navigate between tabs (optional) | Main screens |
| Swipe vertical | Scroll | Scrollable content |
| Pinch | Zoom | Trend charts only |
| Double tap | Zoom to default | Trend charts |

### 6.2 Data Entry

#### UX-INT-010: On-Screen Keyboard
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | On-screen keyboard for text/numeric entry |

**Keyboard Types:**
- Numeric: PIN entry, patient ID (if numeric), vital limits
- Alphanumeric: Patient name, free text notes
- Specialized: IP address, date/time

**Behavior:**
- Keyboard appears when input field focused
- Input field remains visible above keyboard
- Clear button to erase
- Done/Enter button to confirm
- Tap outside to dismiss (with confirmation if data entered)

#### UX-INT-011: Numeric Adjustment Controls
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | Controls for adjusting numeric values |

**For Alarm Limits, Volume, Brightness:**
- Slider control for continuous adjustment
- +/- buttons for precise adjustment
- Direct numeric entry option
- Current value always displayed
- Range limits enforced

```
┌─────────────────────────────────────────┐
│  HR High Limit                          │
│  ┌─────────────────────────────────┐    │
│  │ [-]  ═══════════●═══════  [+]  │    │
│  └─────────────────────────────────┘    │
│           Current: 120 bpm              │
│  Range: 50 - 200 bpm                    │
└─────────────────────────────────────────┘
```

### 6.3 Confirmations

#### UX-INT-020: Destructive Action Confirmation
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Destructive actions require explicit confirmation |

**Actions Requiring Confirmation:**
- Discharge patient
- Clear alarm history
- Reset to defaults
- Delete data
- System shutdown

**Confirmation Dialog:**
- Clear statement of action and consequence
- Distinct Yes/Confirm and No/Cancel buttons
- Default focus on Cancel (prevent accidental confirmation)
- Timeout returns to Cancel (no auto-confirm)

#### UX-INT-021: Implicit Confirmation
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | Some actions apply immediately with undo option |

**Actions with Implicit Confirmation:**
- Alarm limit changes (apply immediately, undo available briefly)
- Display settings (see result immediately, revert option)
- Patient association (apply, but changeable)

---

## 7. Visual Design Requirements

### 7.1 Color Palette

#### UX-VIS-001: Functional Color System
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Colors shall convey consistent meaning |

**Primary Functional Colors:**

| Color | Hex (example) | Usage |
|-------|---------------|-------|
| **Alarm Red** | #FF0000 | High priority alarm, critical |
| **Warning Yellow** | #FFCC00 | Medium priority alarm, caution |
| **Advisory Cyan** | #00CCFF | Low priority alarm, technical |
| **Normal Green** | #00CC00 | Normal/good status |
| **Inactive Gray** | #666666 | Disabled, unavailable |

**Parameter Colors:**

| Parameter | Color | Rationale |
|-----------|-------|-----------|
| ECG/HR | Green | Industry convention |
| SpO2/Pleth | Cyan/Light Blue | Industry convention |
| NIBP | White/Red | Industry convention |
| Temperature | Orange/Amber | Warmth association |
| Respiration | Yellow/White | Differentiation |

#### UX-VIS-002: Background Colors
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Background colors for readability |

| Element | Color | Rationale |
|---------|-------|-----------|
| Screen background | Dark (near black) | Reduces eye strain, highlights vitals |
| Vital parameter box | Slightly lighter dark | Define boundaries |
| Alarm bar (high) | Red background | Maximum prominence |
| Alarm bar (medium) | Yellow/orange background | High prominence |
| Alarm bar (no alarm) | Dark/neutral | Unobtrusive |

#### UX-VIS-003: Color Accessibility
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Color shall not be the only indicator |

**Requirements:**
- All color-coded information also has text/icon indicator
- Shapes and patterns supplement color (e.g., alarm icons)
- Sufficient contrast for color vision deficiency
- Critical states have multiple cues (color + flash + sound)

### 7.2 Typography

#### UX-VIS-010: Font Selection
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Typography optimized for clinical readability |

**Requirements:**
- Sans-serif font family (e.g., Roboto, Inter, or LVGL built-in)
- Clear distinction between similar characters (0/O, 1/l/I)
- Monospace option for numeric values (alignment)
- Embedded in firmware (no external font dependencies)

#### UX-VIS-011: Type Scale
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Consistent typography sizing |

| Level | Usage | Size Range |
|-------|-------|------------|
| Display | Primary vital values | 48-72pt |
| Title | Screen titles, parameter labels | 24-32pt |
| Body | General text, descriptions | 16-18pt |
| Caption | Secondary info, timestamps | 12-14pt |
| Minimum | Legal, footnotes | 10pt (avoid if possible) |

*Note: Actual sizes depend on screen DPI and will be defined in implementation.*

#### UX-VIS-012: Text Legibility
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Text shall be readable under clinical conditions |

**Requirements:**
- Minimum contrast ratio 4.5:1 (WCAG AA)
- Primary vitals contrast ratio 7:1+ (WCAG AAA)
- No text on busy backgrounds
- Adequate line spacing for multi-line text

### 7.3 Iconography

#### UX-VIS-020: Icon System
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Consistent icon usage |

**Icon Requirements:**
- Simple, recognizable shapes
- Consistent stroke weight
- Appropriate size for touch (if interactive)
- Paired with text labels where ambiguous

**Standard Icons:**

| Function | Icon Description |
|----------|------------------|
| Alarm (high) | Bell with exclamation |
| Alarm (silenced) | Bell with slash |
| Settings/Gear | Gear/cog |
| Patient | Person silhouette |
| Trends | Line chart |
| Home/Main | House or heart |
| Lock | Padlock |
| Battery | Battery outline with level |
| WiFi | Signal strength bars |
| Ethernet | Network jack |

### 7.4 Animation and Transitions

#### UX-VIS-030: Animation Principles
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | Animation supports usability, not decoration |

**Guidelines:**
- Animations brief (150-300ms)
- Purpose: indicate state change, guide attention
- Never delay critical information
- Can be disabled in settings
- Alarm flashing is functional, not decorative (per IEC 60601-1-8)

#### UX-VIS-031: Transition Animations
| Attribute | Specification |
|-----------|---------------|
| Priority | P3 |
| Description | Screen transitions |

| Transition | Animation |
|------------|-----------|
| Screen change | Fade or slide (brief) |
| Modal appear | Fade in + slight scale |
| Modal dismiss | Fade out |
| Alarm flash | Per IEC 60601-1-8 rates |

---

## 8. Accessibility Requirements

### 8.1 Visual Accessibility

#### UX-ACC-001: High Contrast Support
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | Interface shall be usable by users with visual impairments |

**Requirements:**
- Default high-contrast color scheme
- No reliance on subtle color differences
- Alternative high-contrast mode option
- Brightness adjustment

#### UX-ACC-002: Text Scaling
| Attribute | Specification |
|-----------|---------------|
| Priority | P3 |
| Description | Text size adjustment |

**Requirements:**
- Default text sizes adequate for most users
- Optional larger text mode for accessibility
- Layout accommodates larger text without breaking

### 8.2 Motor Accessibility

#### UX-ACC-010: Touch Accommodation
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | Interface shall accommodate users with motor impairments |

**Requirements:**
- Large touch targets (per UX-INT-001)
- No gestures required for critical functions (tap alternatives)
- No time-limited interactions for critical functions
- Adjustable touch sensitivity (if hardware supports)

### 8.3 Glove Compatibility

#### UX-ACC-020: Gloved Operation
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Interface shall be operable with medical gloves |

**Requirements:**
- Touch targets sized for gloved fingers (larger)
- No fine-motor precision required
- Testing with common glove types (nitrile, latex)
- Hardware touch sensitivity supports gloves

---

## 9. Screen-Specific Mockup Guidelines

### 9.1 Main Vitals Screen Layout Variants

#### Single Patient Mode - Standard
```
┌─────────────────────────────────────────────────────────────────────┐
│ ▲ ALARM: HR HIGH - 156 bpm                          [ACK] │ 14:32  │
├───────────────────────────────────────────────┬─────────────────────┤
│                                               │    HR        ❤      │
│    ┌─────────────────────────────────────┐    │         72         │
│    │ ECG  II  25mm/s                     │    │        bpm         │
│    │ ╭─╮   ╭─╮   ╭─╮   ╭─╮   ╭─╮        │    │   50 ◄──────► 120  │
│    │─╯ ╰───╯ ╰───╯ ╰───╯ ╰───╯ ╰────    │    ├─────────────────────┤
│    └─────────────────────────────────────┘    │   SpO2       💧     │
│                                               │         98         │
│    ┌─────────────────────────────────────┐    │          %         │
│    │ Pleth                               │    │   90 ◄──────► 100  │
│    │ ∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿   │    ├─────────────────────┤
│    └─────────────────────────────────────┘    │   NIBP      [START] │
│                                               │     120/80         │
│                                               │       (93)         │
│                                               │      mmHg          │
│                                               │     5 min ago      │
│                                               ├──────────┬──────────┤
│                                               │  Temp    │    RR   │
│                                               │  36.8    │    16   │
│                                               │   °C     │   /min  │
├───────────────────────────────────────────────┴──────────┴──────────┤
│  Pt: Sharma, R (302-A)      │ [Trends] [Alarms] [Patient] [⚙]      │
└─────────────────────────────────────────────────────────────────────┘
```

#### Dual Patient Mode
```
┌─────────────────────────────────────────────────────────────────────┐
│ ▲ ALARM: Bed A - SpO2 LOW                           [ACK] │ 14:32  │
├────────────────────────────────┬────────────────────────────────────┤
│        BED A - Sharma, R       │        BED B - Patel, M            │
├────────────────────────────────┼────────────────────────────────────┤
│  ECG ╭─╮   ╭─╮   ╭─╮          │  ECG ╭─╮   ╭─╮   ╭─╮              │
│  ────╯ ╰───╯ ╰───╯ ╰──        │  ────╯ ╰───╯ ╰───╯ ╰──            │
│                                │                                    │
│   HR      SpO2     NIBP       │   HR      SpO2     NIBP           │
│   72       88      118/76     │   84       97      132/84         │
│   bpm       %       mmHg      │   bpm       %       mmHg          │
│                                │                                    │
│  Temp: 36.8°C   RR: 16/min   │  Temp: 37.2°C   RR: 18/min       │
├────────────────────────────────┴────────────────────────────────────┤
│  [A] Active      │ [Trends] [Alarms] [Patient] [⚙]                 │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 10. LVGL Implementation Considerations

### 10.1 LVGL-Specific Guidelines

#### UX-LVGL-001: Widget Selection
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Use appropriate LVGL widgets |

**Recommended Widgets:**
- `lv_label` - Vital sign values, text
- `lv_btn` - Interactive buttons
- `lv_chart` - Waveforms, trends
- `lv_bar` - Progress, levels
- `lv_slider` - Adjustments
- `lv_keyboard` - On-screen input
- `lv_tabview` - Screen navigation
- `lv_msgbox` - Confirmations, alerts

#### UX-LVGL-002: Custom Widget Development
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | Custom widgets for specialized needs |

**Custom Widgets Needed:**
- Vital parameter display (combined label + value + limits + status)
- Waveform display (real-time scrolling trace)
- Alarm bar (priority-aware, flashing)
- Numeric entry (large digits, medical context)

#### UX-LVGL-003: Layout Management
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Responsive layout approach in LVGL |

**Approach:**
- Use `lv_obj_set_flex_flow()` for flexible layouts
- Define size as percentages where possible
- Create layout variants for different screen classes
- Runtime screen size detection and layout selection

---

## Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2024 | — | Initial draft |

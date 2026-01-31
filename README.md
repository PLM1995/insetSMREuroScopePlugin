# Inset SMR EuroScope Plugin

Note: This plugin is still in its early stages, and is provided with no warrenty. For any bugs observed, please raise an Issue.

This plugin allows the display of a miniature SMR window on other radar views within EuroScope. It is designed for use with the UK Controller Pack by default. The plugin .dll and config .json files should be placed in the folder "%APPDATA%/EuroScope/UK/Data/Plugin/InsetSMR/".

Once loaded, ensure the plugin can draw on the display type you need, such as the "SMR Radar Display", "Standards ES Radar Screen" or both.

The supported commands (accessed through the EuroScope command line) are as follows. All are case in-sensitive:

- ".InsetSMR Airport ICAO" Sets the view to the whole airport area of the ICAO specified, assuming the airport is set up in the config.json file properly.
- ".InsetSMR Holding ICAO RUNWAY" Sets the the view to the runway holding area of the ICAO and RUNWAY specified, assuming these are set up in the config.json file properly.
- ".InsetSMR Size SCALE" Sets the size of the inset SMR window, SCALE must be an integar from 1 (smallest) to 9 (biggest). The default is 3.
- ".InsetSMR Dataline" Toggles the DataLine on the tag (showing the Aircraft Type and Assigned SID). Note this relies on the EuroScope SID selection, so the correct runway must be selected in EuroScope for the Data to be accurate.
- ".InsetSMR Hide" Hides the inset SMR all together (equivalent to clicking the 'X' in the top-right of the window).
- ".InsetSMR Show" Shows the inset SMR again once hidden.
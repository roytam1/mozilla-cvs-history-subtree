//
//  TestingPref.h
//  Testing
//
//  Created by Simon Fraser on Wed Jun 19 2002.
//  Copyright (c) 2000 __MyCompanyName__. All rights reserved.
//

#import <PreferencePaneBase.h>

@interface OrgMozillaChimeraPreferenceAppearance : PreferencePaneBase 
{
  IBOutlet NSTabView 		*tabView;

  IBOutlet NSButton 		*checkboxUnderlineLinks;
  IBOutlet NSButton 		*checkboxUseMyColors;
  IBOutlet NSColorWell 	*colorwellBackgroundColor;
  IBOutlet NSColorWell 	*colorwellTextColor;
  IBOutlet NSColorWell 	*colorwellUnvisitedLinks;
  IBOutlet NSColorWell 	*colorwellVisitedLinks;  

  IBOutlet NSMatrix		 	 *matrixChooseFont;
  IBOutlet NSPopUpButton *popupFontRegion;

  IBOutlet NSTextField	*fontSampleProportional;
  IBOutlet NSTextField	*fontSampleMonospace;

  IBOutlet NSTextField	*proportionalSampleLabel;

  // advanced panel stuff
  IBOutlet NSPanel       *advancedFontsDialog;
  IBOutlet NSPopUpButton *serifFontPopup;
  IBOutlet NSPopUpButton *sansSerifFontPopup;
  IBOutlet NSPopUpButton *cursiveFontPopup;
  IBOutlet NSPopUpButton *fantasyFontPopup;

  IBOutlet NSTextField   *advancedFontsLabel;

  IBOutlet NSMatrix      *defaultFontMatrix;
  
  NSArray								*regionMappingTable;
  NSString              *defaultFontType;
}

- (void)mainViewDidLoad;

- (IBAction)buttonClicked:(id)sender; 
- (IBAction)colorChanged:(id)sender;

- (IBAction)fontChoiceButtonClicked:(id)sender;
- (IBAction)fontRegionPopupClicked:(id)sender;

- (IBAction)showAdvancedFontsDialog:(id)sender;
- (IBAction)advancedFontsDone:(id)sender;

@end

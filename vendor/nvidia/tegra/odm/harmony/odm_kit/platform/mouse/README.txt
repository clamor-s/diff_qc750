
/**********************************************************************/
 * Touchpad ODM driver
/**********************************************************************/


1. To build touchpad ODM with ECI when it's in place set ECI_ENABLE macro to 1.

2. And modify 
   "\main\customers\nvidia\firefly\odm_kit\platform\touchpad\GNUmakefile"
   to link to ECI library.


** After step (1) and (2) touchpad driver is ready for validation and testing on the 
   firefly platform.
 
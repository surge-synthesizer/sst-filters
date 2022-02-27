# SST Filters

Work in progress... Come back soon!

---

List of API changes (relative to original Surge code):
- Pitch: A440 = 69 instead of 0?
- FilterCoefficientMaker uses a full precision method for db_to_linear, in place of a lookup table
- LPMoog filter uses sub-type template argument instead of f->WP
- DiodeLadder filter uses sub-type template argument instead of f->WP
- CutoffWarp filter uses sub-type template argument instead of f->WP
- ResonanceWarp filter uses sub-type template argument instead of f->WP
- TriPole filter uses sub-type template argument instead of f->WP
- Comb filters use a static global sinc table (not sure if that's particularly safe)

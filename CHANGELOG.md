
### master (unreleased)
Introduces a breaking change for the trigger inputs. They are now configured in
the frequency inputs section, and the pins dedicated to triggers have moved to
port A.

- Implement check engine light
- Implement cold start cranking enrich
- Record frequency and trigger edges with DMA, allowing higher rpm operation
- Implement dwell controlled by battery voltage
- Better ADC strategy:
  - Faster and more deterministic sampling
  - Implement customizable averaging windows based on crank angles

### 1.1.0 (2019 Dec 14)
Introduces a breaking change with the console format for tables, as well as
various fixes and features
- Fix various console bugs
- Implement two-output test trigger
- Fix scheduler issue resulting in occasional ignition misses


### 1.0.0 (2019 Nov 10)
Initial release of viaems

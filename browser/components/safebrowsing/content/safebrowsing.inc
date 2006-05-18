<!-- browser.xul overlay
     included from browser/base/content/browser.xul -->

    <!-- This dims out the browser content -->

    <vbox id="safebrowsing-dim-area-transp" hidden="true">
      <spacer flex="1" />
    </vbox>

    <!-- This is the spike aka tail on top of the warning -->

    <hbox id="safebrowsing-palm-message-tail-container" hidden="true" 
     pack="end">
      <image id="safebrowsing-palm-message-tail"
      src="chrome://browser/skin/safebrowsing/tail.png" />
    </hbox>

    <!-- This is the phishing afterload warning message -->

    <vbox id="safebrowsing-palm-message" hidden="true" pack="center"
    class="safebrowsing-palm-fixed-width">

      <!-- This is the main warning area -->

      <vbox id="safebrowsing-palm-message-content" 
      class="safebrowsing-palm-fixed-width safebrowsing-palm-message-bubble">

        <!-- Top line -->

        <hbox id="safebrowsing-palm-message-titlebox"
        class="safebrowsing-palm-fixed-width">
          <image
          src="chrome://browser/skin/safebrowsing/warning24x24.png" 
          id="safebrowsing-palm-title-icon" />
          <description id="safebrowsing-palm-message-title"
          class="safebrowsing-palm-title"
          value="&safeb.palm.warning.heading;" />
          <spacer flex="1" />
          <vbox pack="start" align="start">
            <box onclick="goDoCommand('safebrowsing-decline-warning')"><image 
            src="chrome://browser/skin/safebrowsing/close16x16.png"
            id="safebrowsing-palm-close" /></box>
            <spacer flex="1" />
          </vbox>
        </hbox>

        <!-- Content area: short warning -->

        <description id="safebrowsing-palm-content" 
          class="safebrowsing-paragraph">
          &safeb.palm.message.part1; 
          <label class="text-link plain"
            statustext="&safeb.palm.showmore.statustext;"
            onmouseover="SB_setStatus(this.getAttribute('statustext'))"
            onmouseout="SB_clearStatus()"
            id="safebrowsing-palm-showmore-link"
            onclick="goDoCommand('safebrowsing-palm-showmore')"
            value="&safeb.palm.message.link1.more;" />
        </description>

        <!-- Content area: rest of the warning, revealed if the user
             hits the more link -->

        <vbox id="safebrowsing-palm-extended-message" hidden="true"
          class="safebrowsing-palm-fixed-width">

          <description class="safebrowsing-palm-paragraph">
            &safeb.palm.message.part2;
            <label class="text-link plain"
              id="safebrowsing-palm-faq-link"
              onmouseover="SB_setStatusFor(this.id)"
              onmouseout="SB_clearStatus()"
              onclick="goDoCommand('safebrowsing-palm-phishingfaq')"
              value="&safeb.palm.message.link2.phishing.faq;" />
            &safeb.palm.message.part3;
            <label class="text-link plain"
              id="safebrowsing-palm-phishingorg-link"
              onmouseover="SB_setStatusFor(this.id)"
              onmouseout="SB_clearStatus()"
              onclick="goDoCommand('safebrowsing-palm-phishingorg')"
              value="&safeb.palm.message.link3.antiphishing.org;" />&safeb.palm.message.part3.dot;
          </description>

          <description class="safebrowsing-palm-paragraph">
            &safeb.palm.message.part4;
            <label class="text-link plain"
              id="safebrowsing-palm-fraudpage-link"
              onmouseover="SB_setStatusFor(this.id)"
              onmouseout="SB_clearStatus()"
              onclick="goDoCommand('safebrowsing-palm-fraudpage')"
              value="&safeb.palm.message.link4.gwghome;" />&safeb.palm.message.part4.dot;
          </description>
          <description class="safebrowsing-palm-paragraph">
            &safeb.palm.message.part5;
            <label
              class="text-link plain"
              id="safebrowsing-palm-falsepositive-link"
              onmouseover="SB_setStatusFor(this.id)"
              onmouseout="SB_clearStatus()"
              onclick="goDoCommand('safebrowsing-palm-falsepositive')"
              value="&safeb.palm.message.link5.report;" />&safeb.palm.message.part5.dot;
          </description>
        </vbox>

        <!-- Main action links -->
        <hbox id="safebrowsing-palm-message-actionbox"
         class="safebrowsing-palm-fixed-width"
         align="center" crop="none">
         <label
           class="safebrowsing-palm-bigtext text-link plain"
           statustext="&safeb.palm.accept.statustext;"
           id="safebrowsing-palm-accept-link"
           onmouseover="SB_setStatus(this.getAttribute('statustext'))"
           onmouseout="SB_clearStatus()"
           onclick="goDoCommand('safebrowsing-accept-warning')"
           value="&safeb.palm.accept.label;" />
         <spacer flex="1"/>
         <label
           class="safebrowsing-palm-bigtext text-link plain"
           statustext="&safeb.palm.decline.statustext;"
           id="safebrowsing-palm-decline-link"
           onmouseover="SB_setStatus(this.getAttribute('statustext'))"
           onmouseout="SB_clearStatus()"
           onclick="goDoCommand('safebrowsing-decline-warning')"
           value="&safeb.palm.decline.label;" />
        </hbox>

        <!-- Footer -->

        <hbox align="center" class="safebrowsing-palm-fixed-width">
          <!-- branding logo
          <image id="safebrowsing-palm-logo"
          tooltiptext="&safeb.palm.provider.logo.tooltip;"
          src="chrome://browser/skin/safebrowsing/logo.png" />
          -->
          <spacer flex="1" />
          <description>
            [
            <label id="safebrowsing-palm-report-link"
              onmouseover="SB_setStatusFor(this.id)"
              onmouseout="SB_clearStatus()"
              class="safebrowsing-palm-smalltext text-link plain"
              onclick="goDoCommand('safebrowsing-submit-generic-phishing')"
              value="&safeb.palm.sendreport.label;" />
            ]
          </description>
        </hbox>
      </vbox>
    </vbox>

    <!-- We position things over a canvas; to do so, we need a stack -->

    <stack id="safebrowsing-content-stack" insertbefore="status-bar" 
     hidden="true">

      <!-- The canvas goes here as the first child, but since it takes
           up so much memory and is rarely used, we add and remove it 
           dynamically -->

      <!-- This dims out the browser content -->
 
      <vbox id="safebrowsing-dim-area-canvas" hidden="true" flex="1">
      </vbox>

    </stack>

<implementation>
  <property name="isZapFormWidget" readonly="true" onget="return true;"/>
  <!--
       State word composed as follows:
       0x0001 : attribute 'invalid' == true
       0x0002 : attribute 'modified' == true
    -->
  <property name="state">
    <getter>
    <![CDATA[
      var rv = 0;
      if (this.getAttribute('invalid')=='true')
        rv |= 0x0001;
      if (this.getAttribute('modified')=='true')
        rv |= 0x0002;
      return rv;
    ]]>
    </getter>
    <setter>
    <![CDATA[
      var oldState = this.state;
      if (val != oldState) {
        this.setAttribute('invalid', (val & 0x0001) ? 'true' : 'false');
        this.setAttribute('modified', (val & 0x0002) ? 'true' : 'false');

        if (window.widgetStateChanged)
          window.widgetStateChanged(this, oldState, val);
      }
    ]]>
    </setter>
  </property>

  <property name="validator" readonly="true">
    <getter>
    <![CDATA[
      if (!this._validator) {
        var vexpr = this.getAttribute('validator');
        if (!vexpr) vexpr = "function() {return true;}";
        this._validator = eval(vexpr);
      }
      return this._validator;
    ]]>
    </getter>
  </property>
</implementation>
<handlers>
  <handler event="input">
  <![CDATA[
    var newState = 0x0002; // modified
    if (this.defaultVal != undefined) {
      if (this.value == this.defaultVal)
        newState = 0x0000; // not modfied
    }
    if (this.validator(this.value))
      newState &= ~0x0001;
    else
      newState |= 0x0001;
    this.state = newState;
  ]]>
  </handler>
</handlers>



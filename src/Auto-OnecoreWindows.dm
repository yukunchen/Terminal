<!--
 <copyright file="Auto-OnecoreWindows.dm" company="Microsoft">
     Copyright (c) Microsoft Corporation. All rights reserved.
 </copyright>
-->
<DominoModule xmlns="http://schemas.microsoft.com/domino/0.5">
  <Module>
    <Identity>MsWin.OnecoreWindows</Identity>
    <Partial>true</Partial>
    <SpecFiles>
      <Item>propsheet\autogen.ds</Item>
      <Item>propslib\autogen.ds</Item>
      <Item>testlist\autogen.ds</Item>
      <Item>tsf\autogen.ds</Item>
    </SpecFiles>
  </Module>
  <Import>host\Auto-OnecoreWindows.dm</Import>
  <Import>interactivity\Auto-OnecoreWindows.dm</Import>
  <Import>renderer\Auto-OnecoreWindows.dm</Import>
  <Import>server\Auto-OnecoreWindows.dm</Import>
  <Import>terminal\Auto-OnecoreWindows.dm</Import>
  <Import>tools\Auto-OnecoreWindows.dm</Import>
  <Import>types\Auto-OnecoreWindows.dm</Import>
</DominoModule>
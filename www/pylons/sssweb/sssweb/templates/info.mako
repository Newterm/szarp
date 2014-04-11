<%inherit file="base.mako"/>

<div class="info">
% if c.message is not UNDEFINED:
  <p>${c.message}</p>
% endif
  ${h.form(url.current(action=c.action,controller=c.controller), method='get')}
  ${h.submit('submit', _('Ok'))}
  ${h.end_form()}
</div>


(function ($) {

  Drupal.behaviors.activeClass = {
    attach: function (context, settings) {
      $('#edit-social-media-page :input:checked').closest('.form-item').addClass('active');
      $('#edit-social-media-page :input').once('activeClass').click(function () {
        $(this).closest('.form-item').toggleClass('active');
      })
    }
  };

})(jQuery);
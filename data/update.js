document.addEventListener('DOMContentLoaded', function () {
    // Handle Firmware Update Form
    $('#upload_form').submit(function (e) {
        e.preventDefault();
        var form = $('#upload_form')[0];
        var data = new FormData(form);
        console.log("Form data ready, sending to /updateFirmware...");

        $.ajax({
            url: '/updateFirmware',
            type: 'POST',
            data: data,
            contentType: false,
            processData: false,
            xhr: function () {
                var xhr = new window.XMLHttpRequest();
                xhr.upload.addEventListener('progress', function (evt) {
                    if (evt.lengthComputable) {
                        var per = evt.loaded / evt.total;
                        console.log(`Upload progress: ${Math.round(per * 100)}%`);
                        $('#prg').html('progress: ' + Math.round(per * 100) + '%');
                    }
                }, false);
                return xhr;
            },
            success: function (response) {
                console.log('Update successful:', response);
                $('#prg').html('Update complete; device will reboot in 10 seconds!');
            },
            error: function (xhr, status, error) {
                console.error('Update failed:', error);
                $('#prg').html('Update failed. Please try again.');
            }
        });
    });

    // Handle Filesystem Update Form
    $('#upload_filesystem_form').submit(function (e) {
        e.preventDefault();
        var form = $('#upload_filesystem_form')[0];
        var data = new FormData(form);
        console.log("Filesystem data ready, sending to /updateFilesystem...");

        $.ajax({
            url: '/updateFilesystem',
            type: 'POST',
            data: data,
            contentType: false,
            processData: false,
            xhr: function () {
                var xhr = new window.XMLHttpRequest();
                xhr.upload.addEventListener('progress', function (evt) {
                    if (evt.lengthComputable) {
                        var per = evt.loaded / evt.total;
                        console.log(`Filesystem upload progress: ${Math.round(per * 100)}%`);
                        $('#fs_prg').html('Progress: ' + Math.round(per * 100) + '%');
                    }
                }, false);
                return xhr;
            },
            success: function (response) {
                console.log('Filesystem update successful:', response);
                $('#fs_prg').html('Update complete; device will reboot in 10 seconds!');
            },
            error: function (xhr, status, error) {
                console.error('Filesystem update failed:', error);
                $('#fs_prg').html('Update failed. Please try again.');
            }
        });
    });

    // Handle Individual File Upload Form
    $('#upload_file_form').submit(function (e) {
        e.preventDefault();
        var form = $('#upload_file_form')[0];
        var data = new FormData(form);
        console.log("Uploading file to /uploadFile...");

        $.ajax({
            url: '/uploadFile',
            type: 'POST',
            data: data,
            contentType: false,
            processData: false,
            xhr: function () {
                var xhr = new window.XMLHttpRequest();
                xhr.upload.addEventListener('progress', function (evt) {
                    if (evt.lengthComputable) {
                        var per = evt.loaded / evt.total;
                        console.log(`File upload progress: ${Math.round(per * 100)}%`);
                        $('#file_prg').html('Progress: ' + Math.round(per * 100) + '%');
                    }
                }, false);
                return xhr;
            },
            success: function (response) {
                console.log('File upload successful:', response);
                $('#file_prg').html('File uploaded successfully!');
            },
            error: function (xhr, status, error) {
                console.error('File upload failed:', error);
                $('#file_prg').html('Upload failed: ' + error);
            }
        });
    });
});
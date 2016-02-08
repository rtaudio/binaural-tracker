% spk is 1-channel
function [  ] = plot_match( ref, spk, deltaN )

    ref_matched = ref(1:(end-round(deltaN)));
    spk_matched = spk((1+round(deltaN)):end);
    
    % plot ref matched with speakers
    plot([ref_matched spk_matched(1,:)' spk_matched(2,:)']);
    legend('sweep','spk1','spk2');
    grid on;
end

